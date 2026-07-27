# 実装計画：USB給電検出によるパワーオフ抑制

## 1. 目的

M5Stack Basic（IP5306搭載）において、USB給電中（充電中または満充電）は
自動パワーオフを抑制する機能を追加する。

現在のパワーオフ条件（5分間操作なし + 大気圧下）に加えて、
「USB未接続（充電なし）」を必須条件として追加する。

---

## 2. 現状分析

### 現状のパワーオフロジック（`src/main.cpp` L245–255）

```cpp
// 減圧測定中はリセット
if (current_prs < POWER_OFF_PRESSURE_THRESHOLD_HPA) {
    sleepTimer.start();   // ← タイマーリセット
}

// 5分経過 かつ 大気圧下でパワーオフ
if (sleepTimer.hasExpired()) {
    if (current_prs >= POWER_OFF_PRESSURE_THRESHOLD_HPA) {
        M5.Power.powerOff();
    }
}
```

### `OneShotTrigger_m` の動作（`include/util.h` L141–171）

| メソッド | 動作 |
|---------|------|
| `start()` | `prev = millis()` にセット、タイマー開始（リセット） |
| `hasExpired()` | 経過時間 >= delay なら `true` を1回だけ返す |
| `stop()` | タイマーを停止する |

### IP5306 レジスタ（I2Cアドレス: `0x75`）

| ビット | 意味 |
|--------|------|
| `0x70` bit3 = 1 | 充電中 |
| `0x70` bit2 = 1 | 満充電（USB接続中） |
| 両方 0 | USB未接続（バッテリー放電中） |

### IP5306 の満充電サイクル特性

満充電後、IP5306 はトリクル充電の ON/OFF サイクルを繰り返す。
5分間の間に必ず充電フラグ（bit3）が立つタイミングがあるため、
「5分間で1回でも充電中を検出」という要件は IP5306 との相性が良い。

---

## 3. 提案される変更

### 変更ファイル

- **`src/main.cpp`** のみ（`util.h` への変更なし）

### 追加する関数

```cpp
// IP5306のI2Cレジスタを読み、充電中または満充電なら true を返す
bool isCharging() {
    Wire.beginTransmission(0x75);   // IP5306 I2Cアドレス
    Wire.write(0x70);               // ステータスレジスタ
    if (Wire.endTransmission(false) != 0) return false;
    Wire.requestFrom(0x75, 1);
    if (!Wire.available()) return false;
    uint8_t reg = Wire.read();
    return (reg & 0x08) || (reg & 0x04); // bit3: 充電中, bit2: 満充電
}
```

**配置場所:** `initBMP585()` の直前（グローバル関数として定義）

### パワーオフロジックの変更

#### 変更前（L245–255）

```cpp
// 減圧測定中はリセット
if (current_prs < POWER_OFF_PRESSURE_THRESHOLD_HPA) {
    sleepTimer.start();
}

// 5分経過 かつ 大気圧下でパワーオフ
if (sleepTimer.hasExpired()) {
    if (current_prs >= POWER_OFF_PRESSURE_THRESHOLD_HPA) {
        M5.Power.powerOff();
    }
}
```

#### 変更後

```cpp
// 減圧測定中はリセット
if (current_prs < POWER_OFF_PRESSURE_THRESHOLD_HPA) {
    sleepTimer.start();
}

// 5分経過 かつ 大気圧下でパワーオフ
if (sleepTimer.hasExpired()) {
    if (current_prs >= POWER_OFF_PRESSURE_THRESHOLD_HPA) {
        M5.Power.powerOff();
    }
}
```

#### updateTimer ブロック内に追加（L281付近）

```cpp
// 表示：1秒毎の更新処理
if (updateTimer.hasExpired()) {
    // USB充電中ならスリープタイマーをリセット（パワーオフ抑制）
    if (isCharging()) {
        sleepTimer.start();
    }

    // 測定完了時のみ画面更新
    if (measurement_flag) {
        drawValues();
        measurement_flag = false;
    }
}
```

### 設計の根拠

`isCharging()` を `updateTimer`（1秒ごと）内で呼び出すことで：

1. **I2C通信の効率化** — BMP585 と同じバスを使うが、毎ループではなく1秒ごとに1回のみ実行
2. **スティッキーフラグ不要** — 充電を検出するたびに `sleepTimer.start()` を呼ぶことで「5分間の中で1回でも充電中なら継続」が自然に実現される
3. **既存ロジックへの影響なし** — パワーオフ判定の `if` ブロック自体は変更しない

### 動作フロー（変更後）

```
毎秒（updateTimer）:
    isCharging() == true?
    └─ Yes → sleepTimer.start() でリセット（パワーオフしない）
    └─ No  → 何もしない

5分間充電未検出 + 大気圧 + ボタン操作なし:
    → sleepTimer.hasExpired() == true → M5.Power.powerOff()
```

---

## 4. リスク評価

| リスク | 評価 | 対策 |
|--------|------|------|
| IP5306 I2C通信失敗 | 低 | `endTransmission` 戻り値チェック＋`false`を返す（パワーオフ方向にフェールセーフ） |
| BMP585 との I2C 競合 | 低 | `updateTimer`（1秒）と `pollTimer`（25ms）は別タイミング。競合リスクは低い |
| 満充電 + USB接続の誤検出 | 低 | 5分ウィンドウ内でトリクル充電サイクルが必ず発生するため自然解消 |
| `Wire.begin` の再初期化 | なし | `isCharging()` は既存の `Wire` インスタンスをそのまま使用 |
| バッテリーなし・USB接続の状態 | 中 | IP5306 がない構成では常に `false` を返すが、M5Stack Basic + バッテリーありの前提なので対象外 |

> [!NOTE]
> `isCharging()` がI2C通信失敗で `false` を返す場合、充電中でもパワーオフ方向に動作します。
> これは「充電検出できない = 安全側」のフェールセーフとして妥当です。

---

## 5. 検証計画

| テストケース | 期待動作 |
|-------------|---------|
| USB接続、充電中（バッテリー未満）| 5分後もパワーオフしない |
| USB接続、満充電状態 | 5分後もパワーオフしない（トリクルで検出） |
| USB未接続、バッテリー駆動 | 5分後にパワーオフする |
| USB未接続 + 減圧中 | パワーオフしない（既存動作を維持） |
| USB接続 → 切断 → 5分放置 | 切断後5分でパワーオフする |
| ボタン操作 | 操作後5分でパワーオフ（既存動作を維持） |
