# 減圧装置用 圧力モニター (ENV-Meter)

M5Stack Basic と Adafruit BMP585 を使用した、自由研究などの減圧実験で使う圧力モニターです。

## 特徴
- **減圧実験に最適な圧力モニター**: 減圧装置内の圧力や温度をリアルタイムで安全かつ正確に監視できます。
- **ホールド機能（イベント発生時の圧力記録）**: 実験中に変化や現象が発生した瞬間（イベント発生時）の圧力を、左ボタン（BtnA）ひとつで画面上に固定して記録・確認できます。
- **圧力過剰低下警報機能（容器破損リスク通知）**: 容器内の圧力が下がりすぎると**容器が破損（潰れ・破裂など）する危険性が急増**します。気圧が 700.0 hPa 以下になると間欠ビープ音と赤色画面表示でユーザーへ即座に警告します（ボタン押下で1分間一時消音、710.0 hPa 以上で自動解除）。
- **美しい日本語表示**: `OpenFontRender` を使用し、滑らかで読みやすい日本語フォント（Noto Sans JP）で表示します。
- **チラツキのない表示更新**: 画面全体をリセットせず数値部分のみを書き換えるため、画面がチラつかず快適に読み取れます。
- **省電力設計**: バッテリー駆動を配慮し、5分間操作がない場合は自動的に電源を完全にカットします（本体側面の赤ボタンで再起動）。
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

