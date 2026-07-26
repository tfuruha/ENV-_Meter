# 【備忘録】Adafruit BMP5xxライブラリでデータレディ（dataReady）が常にfalseになる問題と解決策

高精度気圧センサー [Adafruit BMP585](https://www.adafruit.com/product/6413) を [adafruit/Adafruit_BMP5xx](https://github.com/adafruit/Adafruit_BMP5xx) ライブラリを使って制御する際、一定周期で無駄にレジスタを読みに行くのではなく、センサー側でデータが準備できたタイミング（データレディ）で効率よく読み出そうとポーリング実装を行ったところ、思わぬトラップにハマりました。

本記事では、その現象の原因と解決策、および実装時の留意点をまとめます。

## やりたかったこと：dataReady() を使ったスマートなポーリング
センサーのサンプリング周期（ODR）に合わせて無駄なくデータを取得するため、以下のように `dataReady()` メソッドをポーリングし、データが更新された瞬間だけ読み出し（`performReading()`）を行う設計を目指しました。

```cpp
// 25msごとのソフトウェアタイマーなどで実行
if (bmp.dataReady()) {
    if (bmp.performReading()) {
        current_tmp = bmp.temperature;
        current_prs = bmp.pressure;
    }
}
```

しかし、このコードを実行すると **`bmp.dataReady()` がずっと `false` を返し続け、一向にデータが取得できない**という現象が発生しました。

## ハマりどころ：ライブラリ初期化時のトラップ
結論から言うと、原因は**「ライブラリの初期化処理の順序と Bosch API の仕様の組み合わせ」**にありました。

`Adafruit_BMP5xx::begin()` を呼び出すと、内部で `_init()` メソッドが実行されます。この中でデータレディ割り込みを有効化する処理が行われているのですが、Adafruit ライブラリ内部で使われている [Bosch Sensortec BMP5 Sensor API](https://github.com/boschsensortec/BMP5-Sensor-API) の処理は、以下の順序で進みます。

1. `bmp5_int_source_select()` でデータレディ割り込みを有効化（`INT_SOURCE` レジスタをセット）。
2. その直後、ピン設定を行うために Bosch API の `bmp5_configure_interrupt()` を呼び出す。

実は、Bosch の `bmp5_configure_interrupt()` 関数の仕様上、割り込み設定を変更する際に**一時的に `INT_SOURCE` レジスタを強制クリア（全無効化）**してしまいます。
Adafruit のライブラリ側でクリアされたレジスタを元に戻す処理が入っていないため、`begin()` を呼んだ直後のセンサーは**「ハードウェア側でデータレディフラグの発生自体が無効化された状態」**に陥ってしまうのです。

これが `dataReady()` が常に `false` になる根本的な原因でした。

## 解決策：configureInterrupt() で明示的に再設定する
この問題を回避するには、初期化処理の最後に `configureInterrupt()` メソッドを明示的に呼び出し、**データレディフラグを再有効化**します。

また、今回は **INT ピンをどこにも接続していない（未接続）** 状態の基板を使用していたため、[Bosch BMP585 データシート](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp585-ds003.pdf) の推奨に従い、INT ピンがフローティング状態になってノイズや無駄な消費電流を発生させるのを防ぐ設定も同時に行います。

### 正しい初期化コードの例
```cpp
void setup() {
    // ... 前略（Wireの初期化など） ...
    
    if (!bmp.begin(BMP5XX_DEFAULT_ADDRESS, &Wire)) {
        Serial.println("BMP585 not found!");
        while (1);
    }

    // 1. センサーの測定パラメータ設定
    bmp.setTemperatureOversampling(BMP5XX_OVERSAMPLING_4X);
    bmp.setPressureOversampling(BMP5XX_OVERSAMPLING_128X);
    bmp.setIIRFilterCoeff(BMP5XX_IIR_FILTER_COEFF_3);
    bmp.setOutputDataRate(BMP5XX_ODR_10_HZ);
    bmp.setPowerMode(BMP5XX_POWERMODE_NORMAL);

    // 2. ★一番最後に割り込み設定を上書きする★
    bmp.configureInterrupt(
        BMP5XX_INTERRUPT_LATCHED,
        BMP5XX_INTERRUPT_ACTIVE_HIGH,
        BMP5XX_INTERRUPT_PUSH_PULL,     // プッシュプル出力
        BMP5XX_INTERRUPT_DATA_READY,    // データレディを有効化（バグ回避）
        true                            // INTピン出力有効（未接続時の推奨設定）
    );
}
```

この `configureInterrupt()` を追加するだけで、`dataReady()` が期待通り `true` を返すようになります。また、未接続の INT ピンを出力有効（`true`）かつプッシュプル出力に設定することで、センサー内部で電位がHigh/Lowに固定され、動作の安定化と省電力化が図れます。

## 実装時の重要な留意点
上記の設定を行ってポーリング処理を実装する際、以下の2点に注意が必要です。

### 1. `configureInterrupt()` は「一番最後」に呼ぶ
センサーの OSR（オーバーサンプリング）や ODR（出力データレート）、IIR フィルターなどの設定を変更すると、センサー内部でデータや FIFO のフラッシュ処理が走り、その際に**割り込みステータスが意図せずリセットされる**可能性があります。
そのため、`configureInterrupt()` は必ず各種測定設定をすべて完了した**一番最後**に呼び出してください。

### 2. 「Clear-on-read」仕様に注意（ポーリングの排他制御）
BMP585 の割り込みステータスレジスタ（`INT_STATUS`：レジスタ `0x27`）は、**「読み出すと同時にフラグが自動的にクリアされる（Clear-on-read）」**というハードウェア仕様になっています。

`bmp.dataReady()` を呼ぶと内部でこのレジスタを読み出すため、その瞬間にフラグがクリアされます。そのため、「タイマー割り込み内」と「メインループ内」など、**複数箇所で `dataReady()` や `performReading()` を実行すると、一方の呼び出しでフラグが消去され、他方で検出できなくなる（常に false に見える）**原因となります。

データの取得は、`millis()` などを用いたソフトウェアタイマーを使用し、**メインループ内の一箇所のみでステータスを確認する**設計に一本化するのが確実です。

## まとめ
* **Adafruit_BMP5xx ライブラリで `dataReady()` を使う場合**：初期化時の設定クリアを回避するため、`configureInterrupt()` による割り込み再設定が必須。
* **呼び出し順序**：割り込み設定は、必ずすべての測定パラメータ設定の「一番最後」に行う。
* **未接続 INT ピンの扱い**：INT ピンを使わない場合でも、プッシュプル出力として有効化（`true`）して電位を固定するのがデータシートの推奨。
* **排他制御**：データレディフラグは読み出すと自動消去される（Clear-on-read）ため、ステータス確認はコード内の一箇所に一本化する。

同じように BMP585 のデータ取得タイミングや初期化周りで悩んでいる方の参考になれば幸いです！

## 参考リンク・仕様書
* [Adafruit BMP585](https://www.adafruit.com/product/6413)（[スイッチサイエンス 製品ページ](https://www.switch-science.com/products/10949)）
* [Bosch Sensortec BMP5 Sensor API (GitHub)](https://github.com/boschsensortec/BMP5-Sensor-API)
* [Bosch Sensortec BMP585 Data sheet (PDF)](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp585-ds003.pdf)
* [Adafruit BMP5xx Library (GitHub)](https://github.com/adafruit/Adafruit_BMP5xx)



---