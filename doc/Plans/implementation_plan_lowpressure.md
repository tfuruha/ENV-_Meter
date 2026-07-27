# 実装計画書: パワーオフ移行判定条件の追加（気圧条件980hPa以上）

## 1. 目的
パワーオフ（自動電源遮断）移行の判定条件に「気圧測定値が 980.0 hPa 以上」を追加します。
減圧測定中（気圧 < 980.0 hPa）は使用中（作業中）とみなし、オートパワーオフタイマーをリセットすることで、測定中の意図しない電源オフを防止します。

---

## 2. 現状分析（現状 vs 理想の状態）

### 現状 (Current State)
- 最後にボタンを操作してから 5 分（300,000 ms）が経過すると、`sleepTimer.hasExpired()` が `true` となり、気圧状態に関わらず無条件で `M5.Power.powerOff()` が呼び出されます。
- 減圧測定作業中でボタン操作を行わない時間が 5 分続くと、自動で電源が切れ、計測が中断される問題があります。

### 理想の状態 (Ideal State)
- 気圧測定値 `current_prs < 980.0 hPa`（減圧状態）のときは「使用中」と判定し、オートパワーオフタイマー（`sleepTimer`）を継続的にリセットします。
- 減圧測定が終了して気圧が 980.0 hPa 以上（大気圧近傍）に復帰し、かつ 5 分間ボタン操作がない場合のみ、電源を完全に遮断（`M5.Power.powerOff()`）します。

---

## 3. 提案される変更

### 変更対象ファイル
1. **[main.cpp](file:///d:/PlatformIO_Project/ENV-_Meter/src/main.cpp)**
   - パワーオフ判定閾値の定数定義 `POWER_OFF_PRESSURE_THRESHOLD_HPA` (980.0f) を追加。
   - `loop()` 内で減圧中（`current_prs < POWER_OFF_PRESSURE_THRESHOLD_HPA`）の場合に `sleepTimer.start()` を実行し、使用中としてタイマーをリセット。
   - `sleepTimer.hasExpired()` 発生時にも、気圧が閾値以上（`current_prs >= POWER_OFF_PRESSURE_THRESHOLD_HPA`）であることを再確認して `M5.Power.powerOff()` を実行。

2. **[SoftwareDesign.md](file:///d:/PlatformIO_Project/ENV-_Meter/doc/SoftwareDesign.md)**
   - セクション 4.6（省電力機能）の記述を更新し、気圧条件（980 hPa以上で判定、980 hPa未満は使用中としてタイマー延長）の仕様を追記。

---

### 具体的な変更内容（コード構造）

#### `src/main.cpp` の変更案
```cpp
// --- 定数定義の追加 ---
constexpr float POWER_OFF_PRESSURE_THRESHOLD_HPA = 980.0f; // パワーオフ判定用気圧閾値 (hPa)

// --- loop() 内の変更案 ---
void loop() {
  M5.update();
  
  // ... ボタン操作処理 (ボタン押下時に sleepTimer.start() 呼び出し) ...

  // 減圧測定中（980hPa未満）は「使用中」とみなしてパワーオフタイマーをリセット
  if (current_prs < POWER_OFF_PRESSURE_THRESHOLD_HPA) {
    sleepTimer.start();
  }

  // 5分間無操作かつ大気圧下（980hPa以上）の場合にパワーオフ実行
  if (sleepTimer.hasExpired()) {
    if (current_prs >= POWER_OFF_PRESSURE_THRESHOLD_HPA) {
      M5.Power.powerOff();
    }
  }

  // ... 測定・描画処理 ...
}
```

---

## 4. 検証計画

1. **ビルド検証**
   - PlatformIO を用いてコンパイルエラーおよび警告が発生しないことを確認します（`pio run`）。

2. **動作検証シナリオ**
   - **シナリオ 1（大気圧下での自動パワーオフ）:**
     - 気圧が 980.0 hPa 以上の状態で放置し、5 分経過後にデバイスが正常にパワーオフすること。
   - **シナリオ 2（減圧中のパワーオフ防止）:**
     - 気圧を 980.0 hPa 未満（例: 900 hPa や 700 hPa）にし、ボタン操作なしで 5 分以上経過してもパワーオフされず、画面更新と計測が継続されること。
   - **シナリオ 3（減圧復帰後のタイマー動作）:**
     - 減圧測定終了後、気圧が 980.0 hPa 以上に戻ってから 5 分間ボタン操作を行わなかった場合、正常にパワーオフされること。
   - **シナリオ 4（ボタン操作によるリセット維持）:**
     - ボタン押下時に従来の `sleepTimer.start()` が働き、大気圧下での無操作 5 分タイマーがリセットされること。

---

## 5. リスク評価

- **標高や天候の影響:**
  - 標準大気圧は 1013.25 hPa ですが、高地や台風等の低気圧下でも通常大気圧が 980 hPa 未満になることは稀です（標高約 300m 以下では概ね 980 hPa 以上）。980.0 hPa は減圧ラインの閾値として適切です。
- **センサー起動時・エラー時の動作:**
  - 初期化前やセンサーエラー時の `current_prs` の値に起因する予期せぬ電源タイマーリセットや電源オフの固執がないか注意します。エラー時（`error_count >= 5`）の考慮も必要に応じて確認します。
