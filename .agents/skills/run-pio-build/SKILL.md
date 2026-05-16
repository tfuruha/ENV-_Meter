---
name: run-pio-build
description: pioコマンドによるビルドやアップロードが必要になった時に使用する。PlatformIOプロジェクトのビルド、アップロード、テスト実行を行うスキル
---

## 実行方法

### Windows (PowerShell)
```powershell
.agent/skills/run-pio-build/scripts/run-pio-build.ps1 -t build  # or -t upload, -t test, etc.
```

### macOS / Linux (Bash)
```bash
.agent/skills/run-pio-build/scripts/run-pio-build.sh -t build  # or -t upload, -t test, etc.
```

### 引数について
`run-pio-build` スキルは引数として PlatformIO CLI のコマンドをそのまま受け取ります。よく使われる引数は以下の通りです：

- `-t build` または `build`: プロジェクトをビルドします
- `-t upload`: ビルドしたファームウェアをデバイスにアップロードします
- `-t test`: テストコードを実行します
- `-t monitor`: シリアルモニターを開きます
- `-e <environment_name>`: 特定の環境（例: -e esp32dev）を指定して実行します

### 使用例
**ESP32dev 環境でビルドし、アップロードする**
```powershell
.agent/skills/run-pio-build/scripts/run-pio-build.ps1 -e esp32dev -t upload
```

**Nano Every 環境でテストコードを実行する**
```bash
.agent/skills/run-pio-build/scripts/run-pio-build.sh -e nano_every -t test
```