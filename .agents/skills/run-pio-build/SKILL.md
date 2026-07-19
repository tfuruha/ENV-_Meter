---
name: run-pio-build
description: pioコマンドによるビルドやアップロードが必要になった時に使用する。PlatformIOプロジェクトのビルド、アップロード、テスト実行を行うスキル
---

## 実行方法

### Windows (PowerShell)
```powershell
.agents/skills/run-pio-build/scripts/run-pio-build.ps1  # 引数なしでビルド、または -t upload, -t test など
```

### macOS / Linux (Bash)
```bash
.agents/skills/run-pio-build/scripts/run-pio-build.sh  # 引数なしでビルド、または -t upload, -t test など
```

### 引数について
`run-pio-build` スキルは引数として PlatformIO CLI のコマンドをそのまま受け取ります。よく使われる引数は以下の通りです：

- 引数なし: デフォルトのビルドを実行します
- `-t upload`: ビルドしたファームウェアをデバイスにアップロードします
- `-t test`: テストコードを実行します
- `-t monitor`: シリアルモニターを開きます
- `-e <environment_name>`: 特定の環境（例: -e esp32dev）を指定して実行します

### 使用例
**ESP32dev 環境でビルドし、アップロードする**
```powershell
.agents/skills/run-pio-build/scripts/run-pio-build.ps1 -e esp32dev -t upload
```

**Nano Every 環境でテストコードを実行する**
```bash
.agents/skills/run-pio-build/scripts/run-pio-build.sh -e nano_every -t test
```