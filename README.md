# M5Stack ENV-III Meter

M5Stack Basic と Adafruit BMP585 を使用した、高品位な表示の環境メーターです。

## 特徴
- **美しい日本語表示**: `OpenFontRender` を使用し、アンチエイリアスの効いた TrueType フォント（Noto Sans JP）で温度・気圧を表示します。
- **チラツキのない更新**: 画面全体をクリアせず、数値部分のみを差分更新することで、ストレスのない表示更新を実現しています。
- **圧力過剰低下警報機能**: 気圧が 700 hPa 以下になると間欠ビープ音（2kHz）と画面表示（赤字）で警告します。ボタン押下で1分間スヌーズ可能、710 hPa 以上で自動復帰します。
- **ホールド機能**: 左ボタン（BtnA）を押すことで、その時点の測定データ（温度・気圧）を画面に固定（ホールド）できます。再度押下すると解除され、リアルタイム表示に戻ります。
- **省電力設計**: バッテリー駆動を考慮し、5分間ボタン操作がない場合は自動的にデバイスを完全にパワーオフ（電源切断）します。側面の物理電源ボタンを押すことで再起動します。
- **安心のエラー表示**: センサーの脱落や通信エラーを検知し、自動的に `Err` 表示に切り替わります。

## ハードウェア構成
- **本体**: [M5Stack Basic (v2.6)](https://docs.m5stack.com/ja/core/BASIC%20v2.6)
- **センサー**: Adafruit BMP585 (温度・気圧)
- **接続**: Port A (SDA=21, SCL=22) に接続

## 開発環境
- **Framework**: Arduino / PlatformIO
- **主なライブラリ**:
  - `m5stack/M5Unified`
  - `Adafruit_BMP5xx`
  - `takkaO/OpenFontRender`

## セットアップ手順

### 1. フォントデータの準備
本プロジェクトでは `include/binaryttf.h` にフォントデータを配置します。
`works/` ディレクトリ内のスクリプトやツールを使用し、文字セットからサブセットフォント（`NotoSansJP-R_sub.ttf`）を生成・ヘッダー化しています。

使用する文字セットの目安：
` !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~温度気圧℃計測中ホールド解除圧力低下警報スヌーズ`

### 2. ビルドと書き込み
PlatformIO IDE、または付属のスキルスクリプトを使用してビルド・書き込みを行ってください。

**ビルド:**
```powershell
.agents/skills/run-pio-build/scripts/run-pio-build.ps1
```

**書き込み:**
```powershell
.agents/skills/run-pio-build/scripts/run-pio-build.ps1 -t upload
```

## ドキュメント
詳細な仕様や設計については `doc/` ディレクトリを参照してください。
- [仕様概要](doc/SpecSammary.md)
- [ソフトウェア設計書](doc/SoftwareDesign.md)

## ライセンス
本プロジェクトは [MIT ライセンス](LICENSE) のもとで公開されています。詳細は [LICENSE](LICENSE) ファイルを参照してください。

