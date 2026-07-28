#include "AlarmManager.h"
#include "HoldManager.h"
#include "OpenFontRender.h"
#include "binaryttf.h"
#include "util.h"
#include <Adafruit_BMP5xx.h>
#include <M5Unified.h>
#include <Wire.h>

Adafruit_BMP5xx bmp;
OpenFontRender ofr;

// --- 状態管理 ---
constexpr float POWER_OFF_PRESSURE_THRESHOLD_HPA = 980.0f; // パワーオフ判定用気圧閾値 (hPa)

float current_tmp = 0.0; // 温度
float current_prs = 0.0; // 圧力 単位 hPa
int32_t error_count =
    5; // 初期状態はエラー（5回以上）扱いにして Err を表示させる

IntervalTrigger_m updateTimer(1000); // 1秒更新
IntervalTrigger_m pollTimer(25);     // 100ms周期（ODR 10Hzに同期）のポーリング
OneShotTrigger_m sleepTimer(300000); // 5分でパワーオフ
bool measurement_flag = false;

// 管理クラスのインスタンス
AlarmManager alarmMgr;
HoldManager holdMgr;

// IP5306の充電状態（充電中）を判定
bool isCharging() {
  Wire.beginTransmission(0x75);
  Wire.write(0x70);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  Wire.requestFrom(0x75, 1);
  if (!Wire.available()) {
    return false;
  }
  uint8_t reg = Wire.read();
  return (reg & 0x08);// || (reg & 0x04);
}


// BMP585 の初期化と設定
bool initBMP585() {
  Serial.println("Initializing BMP585...");
  // adafruit BMP585のデフォルトアドレスは0x47(BMP5XX_ALTERNATIVE_ADDRESS)
  if (!bmp.begin(BMP5XX_ALTERNATIVE_ADDRESS, &Wire)) {
    Serial.println("BMP585 not found at 0x47, trying 0x46...");
    if (!bmp.begin(BMP5XX_DEFAULT_ADDRESS, &Wire)) {
      Serial.println("Could not find a valid BMP5 sensor, check wiring!");
      return false;
    } else {
      Serial.println("BMP585 found at 0x47");
    }
  } else {
    Serial.println("BMP585 found at 0x46");
  }

  // センサー測定設定
  // 温度：4倍、気圧：128倍、IIRフィルター：3、ODR：10Hz、パワーモード：NORMAL
  bmp.setTemperatureOversampling(BMP5XX_OVERSAMPLING_4X);
  bmp.setPressureOversampling(BMP5XX_OVERSAMPLING_128X);
  bmp.setIIRFilterCoeff(BMP5XX_IIR_FILTER_COEFF_3);
  bmp.setOutputDataRate(BMP5XX_ODR_10_HZ);
  bmp.setPowerMode(BMP5XX_POWERMODE_NORMAL);
  bmp.configureInterrupt(BMP5XX_INTERRUPT_LATCHED, BMP5XX_INTERRUPT_ACTIVE_HIGH,
                         BMP5XX_INTERRUPT_PUSH_PULL,  // プッシュプル出力
                         BMP5XX_INTERRUPT_DATA_READY, // データレディを有効化
                         true // INTピン出力有効 (未接続時のデータシート推奨)
  );

  return true;
}

struct DisplayItem {
  // --- 共通レイアウト定数 ---
  static constexpr int LABEL_X = 20;
  static constexpr int UNIT_X = 240;
  static constexpr int VALUE_X = 120;

  static constexpr int CLEAR_X = 110;
  static constexpr int CLEAR_Y_OFFSET =
      -4; // y_pos に対する消去開始Y座標のオフセット
  static constexpr int CLEAR_WIDTH = 125;
  static constexpr int CLEAR_HEIGHT = 46;
  // -------------------------------------------------------------

  const char *label;
  const char *unit;
  const char *format;
  int y_pos;
  float *value_ptr;
  float scale; // 気圧の100.0除算などに使用（デフォルト1.0）

  void drawStatic(OpenFontRender &ofr) const {
    ofr.drawString(label, LABEL_X, y_pos);
    ofr.drawString(unit, UNIT_X, y_pos);
  }

  void drawValue(OpenFontRender &ofr, bool is_error, float val) const {
    // 定数を用いて消去処理の意図を明確化
    M5.Lcd.fillRect(CLEAR_X, y_pos + CLEAR_Y_OFFSET, CLEAR_WIDTH, CLEAR_HEIGHT,
                    BLACK);

    if (is_error) {
      ofr.drawString("Err", VALUE_X, y_pos);
    } else {
      char buf[16];
      sprintf(buf, format, val / scale);
      ofr.drawString(buf, VALUE_X, y_pos);
    }
  }
};

DisplayItem displayItems[] = {
    {"温度:", "℃", "%4.1f", 55, &current_tmp, 1.0f},
    {"気圧:", "hPa", "%4.0f", 115, &current_prs, 1.0f}};

// 状態表示とボタンラベルの描画処理（宣言順の調整のためプロトタイプ宣言）
void drawStatusAndLabels();
void drawValues();

// 固定部分（項目名・単位）の描画処理
void drawStaticPart() {
  M5.Lcd.clear(BLACK);
  ofr.setFontSize(36);
  ofr.setFontColor(TFT_WHITE);

  for (const auto &item : displayItems) {
    item.drawStatic(ofr);
  }
}

// 数値部分のみの更新処理
void drawValues() {
  ofr.setFontSize(36);
  uint16_t textColor = TFT_CYAN;
  if (alarmMgr.isAlarmActive()) {
    textColor = TFT_RED;
  } else if (alarmMgr.isSnoozed()) {
    textColor = TFT_YELLOW;
  }
  ofr.setFontColor(textColor);
  bool is_error = (error_count >= 5);

  for (const auto &item : displayItems) {
    float val = holdMgr.isHolding()
                    ? ((item.value_ptr == &current_tmp)
                           ? holdMgr.heldTemp()
                           : holdMgr.heldPressure())
                    : *item.value_ptr;
    bool err = holdMgr.isHolding() ? false : is_error;
    item.drawValue(ofr, err, val);
  }
}

// 状態表示とボタンラベルの描画処理
void drawStatusAndLabels() {
  // 状態表示エリア (Y = 0 ～ 40) のクリアと描画
  M5.Lcd.fillRect(0, 0, 320, 40, BLACK);
  ofr.setFontSize(24);
  if (alarmMgr.isAlarmActive()) {
    ofr.setFontColor(TFT_RED);
    ofr.drawString("[圧力低下警報]", 20, 5);
  } else if (alarmMgr.isSnoozed()) {
    ofr.setFontColor(TFT_YELLOW);
    ofr.drawString("[警報スヌーズ]", 20, 5);
  } else if (holdMgr.isHolding()) {
    ofr.setFontColor(TFT_RED);
    ofr.drawString("[ホールド中]", 20, 5);
  } else {
    ofr.setFontColor(TFT_GREEN);
    ofr.drawString("[計測中]", 20, 5);
  }

  // ボタンラベルエリア (Y = 180 ～ 240) のクリアと描画
  M5.Lcd.fillRect(0, 180, 320, 60, BLACK);
  ofr.setFontSize(24);
  ofr.setFontColor(TFT_WHITE);
  if (holdMgr.isHolding()) {
    ofr.drawString("解除", 40, 200);
  } else {
    ofr.drawString("ホールド", 20, 200);
  }
}

void setup() {
  // I2Cは後でWire.beginするので無効化しておく
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;
  cfg.clear_display = true;
  M5.begin(cfg);

  // M5Stack標準のGroveポート(Port A)
  constexpr int SDA_PIN = 21;
  constexpr int SCL_PIN = 22;
  Wire.begin(SDA_PIN, SCL_PIN, 400000U);

  M5.Lcd.fillScreen(BLACK);

  // フォントの初期化
  ofr.loadFont((uint8_t *)NotoSansJP_Regular, NotoSansJP_Regular_len);
  ofr.setDrawer(M5.Lcd);
  ofr.setFontSize(36);
  ofr.setFontColor(TFT_WHITE);

  // 起動画面
  ofr.drawString("Starting...", 80, 100);

  // センサーの初期化待ち
  delay(1000);

  if (!initBMP585()) {
    error_count = 5;
  }

  // タイマー初期化
  updateTimer.init();
  pollTimer.init();
  sleepTimer.start();

  drawStaticPart();
  drawStatusAndLabels();
  drawValues();
}

void loop() {
  M5.update();

  bool btnA_pressed = M5.BtnA.wasPressed();
  bool btnB_pressed = M5.BtnB.wasPressed();
  bool btnC_pressed = M5.BtnC.wasPressed();

  // ボタン操作でタイマーリセット＆処理実行
  if (btnA_pressed || btnB_pressed || btnC_pressed) {
    sleepTimer.start();
    if (alarmMgr.isAlarmActive()) {
      // 警報鳴動中はいずれのボタンを押しても警報解除（スヌーズ）のみを行う
      if (alarmMgr.handleButton()) {
        drawStatusAndLabels();
        drawValues();
      }
    } else {
      if (btnA_pressed) {
        holdMgr.toggle(current_tmp, current_prs);
        drawStatusAndLabels();
        drawValues();
      }
    }
  }

  // 警報機能の更新処理（スヌーズタイムアウト判定・ビープ音発声判定）
  if (alarmMgr.update(current_prs)) {
    drawStatusAndLabels();
    drawValues();
  }

  // 減圧測定中（980hPa未満）は使用中とみなしてタイマーをリセット
  if (current_prs < POWER_OFF_PRESSURE_THRESHOLD_HPA) {
    sleepTimer.start();
  }

  // 5分（300秒 = 300,000ms）経過かつ大気圧下（980hPa以上）でパワーオフ
  if (sleepTimer.hasExpired()) {
    if (current_prs >= POWER_OFF_PRESSURE_THRESHOLD_HPA) {
      M5.Power.powerOff();
    }
  }

  // 測定：25ms周期で直接データを取得（ODR 10Hz）
  if (pollTimer.hasExpired()) {
    if (bmp.dataReady()) {
      if (bmp.performReading()) {
        current_tmp = bmp.temperature;
        current_prs = bmp.pressure; // hPa
        if (error_count > 0) {
          error_count = 0;
        }
        measurement_flag = true;

        if (alarmMgr.check(current_prs)) {
          drawStatusAndLabels();
          drawValues();
        }
      } else {
        if (error_count < 5) {
          error_count++;
        }
      }
    }
  }

  // 表示：1秒毎の更新処理
  if (updateTimer.hasExpired()) {
    // USB給電（充電中または満充電）時はスリープタイマーをリセットしてパワーオフを抑制
    if (isCharging()) {
      sleepTimer.start();
    }

    // 測定完了時のみ画面更新
    if (measurement_flag) {
      drawValues();
      measurement_flag = false;
    }
  }
}

