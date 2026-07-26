# リファクタリング計画: 警報・ホールド機能のクラス化

## 1. 目的

`main.cpp` に散在している警報機能とホールド機能の状態・ロジックをクラスに封じ込め、
コードの見通しと保守性を向上させる。

**対象外（変更しない）:**
- `struct DisplayItem` — すでに整理されている
- センサー初期化 `initBMP585()`
- 描画関数群 (`drawStaticPart`, `drawValues`, `drawStatusAndLabels`)
- タイマー (`updateTimer`, `pollTimer`, `sleepTimer`)

---

## 2. 現状分析

### 問題のある状態変数の散在

| 変数/タイマー | 現在の場所 | 参照箇所数 |
|---|---|---|
| `is_alarm_active` | グローバル (L28) | `checkAlarm()`, `loop()` x3, `drawValues()`, `drawStatusAndLabels()` |
| `is_snoozed` | グローバル (L29) | `checkAlarm()`, `loop()` x3, `drawValues()`, `drawStatusAndLabels()` |
| `snoozeTimer` | グローバル (L30) | `checkAlarm()`, `loop()` x2 |
| `beepTimer` | グローバル (L31) | `checkAlarm()`, `loop()` x2 |
| `is_holding` | グローバル (L23) | `loop()` x3, `drawValues()`, `drawStatusAndLabels()` x2 |
| `held_tmp` | グローバル (L24) | `loop()`, `drawValues()` |
| `held_prs` | グローバル (L25) | `loop()`, `drawValues()` |

### 警報ロジックの分散 (最大の問題)

```
checkAlarm()    → 発報・自動復帰の判定
loop() L247-254 → ボタン押下によるスヌーズ開始
loop() L269-278 → スヌーズタイムアウトの処理
loop() L281-285 → ビープ音の間欠発声
```
4箇所に分散しており、状態遷移を追うのが困難。

---

## 3. 提案する変更

### 3.1 新規作成ファイル

#### `include/AlarmManager.h`

```cpp
#pragma once
#include "util.h"
#include <M5Unified.h>

/**
 * @brief 圧力過剰低下警報機能を管理するクラス
 *
 * 状態遷移:
 *   IDLE --(prs <= 700)--> ALARM --(button)--> SNOOZE
 *     ^                      |                    |
 *     +-(prs >= 710)---------+-(prs >= 710 or timeout)+
 */
class AlarmManager {
public:
    static constexpr float    ALARM_THRESHOLD  = 700.0f;
    static constexpr float    RESET_THRESHOLD  = 710.0f;
    static constexpr uint32_t SNOOZE_MS        = 60000;
    static constexpr uint32_t BEEP_INTERVAL_MS = 500;
    static constexpr uint32_t BEEP_FREQ_HZ     = 2000;
    static constexpr uint32_t BEEP_DURATION_MS = 100;

    AlarmManager();

    /**
     * @brief センサー値が更新されたら呼び出す（旧 checkAlarm()）
     * @param prs 最新の気圧値 [hPa]
     * @return true: 表示の再描画が必要
     */
    [[nodiscard]] bool check(float prs);

    /**
     * @brief 警報鳴動中のボタン押下時に呼び出す
     * @return true: 表示の再描画が必要
     */
    [[nodiscard]] bool handleButton();

    /**
     * @brief ループ毎に呼び出す（スヌーズTO・ビープ制御）
     * @param prs 最新の気圧値 [hPa]（スヌーズTO後の再発報判定に使用）
     * @return true: 表示の再描画が必要
     */
    [[nodiscard]] bool update(float prs);

    bool isAlarmActive() const { return alarmActive_; }
    bool isSnoozed()     const { return snoozed_; }

private:
    void startAlarm_();
    void stopAll_();
    void startSnooze_();

    bool alarmActive_ = false;
    bool snoozed_     = false;
    OneShotTrigger_m  snoozeTimer_;
    IntervalTrigger_m beepTimer_;
};
```

#### `include/HoldManager.h`

```cpp
#pragma once

/**
 * @brief ホールド機能を管理するクラス
 */
class HoldManager {
public:
    /**
     * @brief BtnA押下時に呼び出す。ホールド開始/解除をトグルする。
     * @param currentTmp 現在の温度値
     * @param currentPrs 現在の気圧値
     */
    void toggle(float currentTmp, float currentPrs);

    bool  isHolding()    const { return holding_; }
    float heldTemp()     const { return heldTmp_; }
    float heldPressure() const { return heldPrs_; }

private:
    bool  holding_ = false;
    float heldTmp_ = 0.0f;
    float heldPrs_ = 0.0f;
};
```

---

### 3.2 `src/main.cpp` の変更内容

#### Step A: インクルード追加・グローバル変数の置き換え

```diff
+#include "AlarmManager.h"
+#include "HoldManager.h"

-// ホールド機能用の状態変数
-bool is_holding = false;
-float held_tmp = 0.0;
-float held_prs = 0.0;
-
-// 圧力過剰低下警報機能用の状態変数・タイマー
-bool is_alarm_active = false;
-bool is_snoozed = false;
-OneShotTrigger_m snoozeTimer(60000);
-IntervalTrigger_m beepTimer(500);

+AlarmManager alarmMgr;
+HoldManager  holdMgr;
```

> **制約:** グローバルコンストラクタはハードウェア初期化前に走る。
> `AlarmManager` / `HoldManager` のコンストラクタはタイマー・変数の初期化のみ行い、
> `M5.Speaker` 等の HW API を呼び出さないこと。

#### Step B: `checkAlarm()` 関数の削除 (L113-133)

```diff
-void checkAlarm() {
-  if (current_prs <= 700.0f) {
-    if (!is_alarm_active && !is_snoozed) { ... }
-  } else if (current_prs >= 710.0f) {
-    if (is_alarm_active || is_snoozed) { ... }
-  }
-}
```

#### Step C: `drawValues()` の参照更新

```diff
-  if (is_alarm_active) {
+  if (alarmMgr.isAlarmActive()) {
     textColor = TFT_RED;
-  } else if (is_snoozed) {
+  } else if (alarmMgr.isSnoozed()) {
     textColor = TFT_YELLOW;
   }
...
-    float val = is_holding
-                    ? ((item.value_ptr == &current_tmp) ? held_tmp : held_prs)
-                    : *item.value_ptr;
-    bool err = is_holding ? false : is_error;
+    float val = holdMgr.isHolding()
+                    ? ((item.value_ptr == &current_tmp)
+                           ? holdMgr.heldTemp()
+                           : holdMgr.heldPressure())
+                    : *item.value_ptr;
+    bool err = holdMgr.isHolding() ? false : is_error;
```

#### Step D: `drawStatusAndLabels()` の参照更新

```diff
-  if (is_alarm_active) {
+  if (alarmMgr.isAlarmActive()) {
-  } else if (is_snoozed) {
+  } else if (alarmMgr.isSnoozed()) {
-  } else if (is_holding) {
+  } else if (holdMgr.isHolding()) {
...
-  if (is_holding) {
+  if (holdMgr.isHolding()) {
```

#### Step E: `loop()` ボタン処理の置き換え

```diff
-    if (is_alarm_active) {
-      is_alarm_active = false;
-      is_snoozed = true;
-      snoozeTimer.start();
-      M5.Speaker.stop();
-      drawStatusAndLabels();
-      drawValues();
-    } else {
+    if (alarmMgr.isAlarmActive()) {
+      if (alarmMgr.handleButton()) {
+        drawStatusAndLabels();
+        drawValues();
+      }
+    } else {
       if (btnA_pressed) {
-        is_holding = !is_holding;
-        if (is_holding) {
-          held_tmp = current_tmp;
-          held_prs = current_prs;
-        }
+        holdMgr.toggle(current_tmp, current_prs);
         drawStatusAndLabels();
         drawValues();
       }
     }
```

#### Step F: `loop()` スヌーズTO・ビープ処理の置き換え

```diff
-  if (is_snoozed && snoozeTimer.hasExpired()) {
-    is_snoozed = false;
-    if (current_prs <= 700.0f) {
-      is_alarm_active = true;
-      beepTimer.init();
-      M5.Speaker.tone(2000, 100);
-    }
-    drawStatusAndLabels();
-    drawValues();
-  }
-  if (is_alarm_active && !is_snoozed) {
-    if (beepTimer.hasExpired()) {
-      M5.Speaker.tone(2000, 100);
-    }
-  }
+  if (alarmMgr.update(current_prs)) {
+    drawStatusAndLabels();
+    drawValues();
+  }
```

#### Step G: 計測完了後の呼び出し置き換え

```diff
       measurement_flag = true;
-      checkAlarm();
+      if (alarmMgr.check(current_prs)) {
+        drawStatusAndLabels();
+        drawValues();
+      }
```

> **設計方針の変更点:** 旧 `checkAlarm()` は内部で描画関数を直接呼んでいたが、
> クラス化後は各メソッドが `bool` を返し、描画責任を呼び出し元（`main.cpp`）に残す。
> これによりクラスが描画コンテキストに依存しなくなる。

---

## 4. 実装順序（推奨）

```
Step 1: include/HoldManager.h を新規作成
Step 2: include/AlarmManager.h を新規作成
Step 3: main.cpp を段階変更
  Step 3A: #include追加 + グローバルインスタンス宣言 + 旧変数削除
  Step 3B: drawValues() / drawStatusAndLabels() の参照を更新
  Step 3C: checkAlarm() 関数を削除
  Step 3D: loop() ボタン処理を更新
  Step 3E: loop() スヌーズTO・ビープ処理を更新
  Step 3F: loop() 計測後呼び出しを更新
Step 4: pio run でビルド確認
Step 5: 実機で動作確認（下記検証シナリオ）
```

---

## 5. リスク評価

| リスク | 深刻度 | 対策 |
|---|---|---|
| グローバルコンストラクタでの HW アクセス | **高** | コンストラクタ内は変数初期化のみ。M5 API は呼ばない |
| `check()` / `handleButton()` の戻り値を無視して描画更新漏れ | **中** | `[[nodiscard]]` 属性でコンパイラ警告を発生させる |
| `item.value_ptr == &current_tmp` のポインタ比較が壊れる | **低** | 既存ロジックをそのまま使用するため変化なし |
| ヘッダオンリー実装でのリンク重複定義 | **低** | `inline` 指定 or `.cpp` 分離、`#pragma once` 必須 |
| `AlarmManager::update()` に渡す `current_prs` の鮮度 | **低** | 直前のポーリングループで更新済みの値を渡すため問題なし |

---

## 6. 検証計画

| シナリオ | 期待動作 | 確認方法 |
|---|---|---|
| 通常起動 | `[計測中]`（緑）・シアン数値 | 起動後の画面確認 |
| BtnA → ホールド開始 | `[ホールド中]`（赤）・数値固定 | BtnA押下 |
| ホールド中 BtnA → 解除 | `[計測中]`（緑）・リアルタイム表示 | BtnA再押下 |
| 気圧 <= 700 hPa (模擬) | `[圧力低下警報]`（赤）・ビープ音 | デバッグビルドで強制代入 |
| 警報中 任意ボタン → スヌーズ | `[警報スヌーズ]`（黄）・無音 | ボタン押下 |
| スヌーズ 60秒後・気圧依然低 | 再発報・ビープ再開 | 60秒待機 |
| 気圧 >= 710 hPa | 全状態リセット・`[計測中]` へ | 値回復確認 |
| 5分無操作 | パワーオフ | 放置確認 |
