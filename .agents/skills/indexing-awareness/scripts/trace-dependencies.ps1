param(
    [Parameter(Mandatory=$true, Position=0)]
    [string]$Target,
    [Parameter(Position=1)]
    [string]$SearchDir = "."
)

# --- プロジェクトルートの自動解決 ---
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$ProjectRoot = Resolve-Path (Join-Path $ScriptDir "..\..\..\..")
Set-Location $ProjectRoot

if ($Target -eq "--help" -or $Target -eq "-h") {
    Write-Host "Usage: .\trace-dependencies.ps1 <file-or-symbol> [search-directory]"
    Write-Host ""
    Write-Host "Modes:"
    Write-Host "  File mode:   引数が既存ファイルパスの場合、そのファイルの依存関係を双方向にトレース"
    Write-Host "  Symbol mode: 引数がシンボル名の場合、定義元と使用箇所を検索"
    Write-Host ""
    Write-Host "Examples:"
    Write-Host "  .\trace-dependencies.ps1 src\utils\auth.ts        # ファイルの依存関係"
    Write-Host "  .\trace-dependencies.ps1 handleLogin               # シンボルの検索"
    Write-Host "  .\trace-dependencies.ps1 UserService src\           # 範囲を限定して検索"
    exit
}

# --- 除外パターン (Get-ChildItem 用) ---
$ExcludeDirs = @("node_modules", ".git", "dist", "build", "__pycache__", ".next", ".venv", "target")

# --- ソースコード拡張子 ---
$SourceExts = @("*.ts", "*.tsx", "*.js", "*.jsx", "*.mjs", "*.cjs", "*.py", "*.java", "*.go", "*.rb", "*.rs", "*.vue", "*.svelte", "*.php", "*.cs", "*.swift", "*.kt", "*.kts")

# --- 設定ファイル拡張子 ---
$ConfigExts = @("*.md", "*.json", "*.yaml", "*.yml", "*.toml", "*.xml", "*.cfg", "*.conf", "*.ini", "*.env")

# --- Import/Require パターン (言語横断) ---
$ImportPattern = "(import |require\(|from ['""]|include |#include |use |using )"

Write-Host "🔗 Dependency Trace: $Target"
Write-Host "   Search scope: $SearchDir"
Write-Host "============================================================"

if (Test-Path $Target -PathType Leaf) {
    # =========================================================
    # FILE MODE
    # =========================================================
    $filename = Split-Path $Target -Leaf
    $filenameNoExt = [System.IO.Path]::GetFileNameWithoutExtension($filename)

    Write-Host "`n📥 Forward Dependencies (このファイルが参照しているもの):"
    Write-Host "---"
    $forward = Select-String -Path $Target -Pattern $ImportPattern -ErrorAction SilentlyContinue
    if ($forward) { $forward | Select-Object -ExpandProperty Line | ForEach-Object { Write-Host $_ } } else { Write-Host "   (none found)" }

    Write-Host "`n📤 Reverse Dependencies (このファイルを参照しているもの):"
    Write-Host "---"
    $reverse = Get-ChildItem -Path $SearchDir -Include $SourceExts -Recurse -File -ErrorAction SilentlyContinue | Where-Object {
        $exclude = $false
        foreach ($d in $ExcludeDirs) { if ($_.FullName -match "\\$d\\") { $exclude = $true; break } }
        -not $exclude
    } | Select-String -Pattern "($filenameNoExt|$filename)" -ErrorAction SilentlyContinue | Where-Object { $_.Path -notmatch [regex]::Escape($Target) } | Select-Object -First 30
    if ($reverse) { $reverse | ForEach-Object { Write-Host "$($_.Filename):$($_.LineNumber): $($_.Line.Trim())" } } else { Write-Host "   (none found)" }

    Write-Host "`n🔍 Config/Doc References (設定・ドキュメントからの参照):"
    Write-Host "---"
    $refs = Get-ChildItem -Path $SearchDir -Include $ConfigExts -Recurse -File -ErrorAction SilentlyContinue | Where-Object {
        $exclude = $false
        foreach ($d in $ExcludeDirs) { if ($_.FullName -match "\\$d\\") { $exclude = $true; break } }
        -not $exclude
    } | Select-String -Pattern $filename -ErrorAction SilentlyContinue | Where-Object { $_.Path -notmatch [regex]::Escape($Target) } | Select-Object -First 20
    if ($refs) { $refs | ForEach-Object { Write-Host "$($_.Filename):$($_.LineNumber): $($_.Line.Trim())" } } else { Write-Host "   (none found)" }

} else {
    # =========================================================
    # SYMBOL MODE
    # =========================================================

    Write-Host "`n📌 Definitions (定義元):"
    Write-Host "---"
    $DefPattern = "(function\s+$Target|const\s+$Target|let\s+$Target|var\s+$Target|class\s+$Target|interface\s+$Target|type\s+$Target|enum\s+$Target|def\s+$Target|fn\s+$Target|func\s+$Target|struct\s+$Target|trait\s+$Target|export\s+(default\s+)?(function|class|const|let|var)\s+$Target)"
    $defs = Get-ChildItem -Path $SearchDir -Include $SourceExts -Recurse -File -ErrorAction SilentlyContinue | Where-Object {
        $exclude = $false
        foreach ($d in $ExcludeDirs) { if ($_.FullName -match "\\$d\\") { $exclude = $true; break } }
        -not $exclude
    } | Select-String -Pattern $DefPattern -ErrorAction SilentlyContinue | Select-Object -First 20
    if ($defs) { $defs | ForEach-Object { Write-Host "$($_.Filename):$($_.LineNumber): $($_.Line.Trim())" } } else { Write-Host "   (none found)" }

    Write-Host "`n📎 Usages (使用箇所):"
    Write-Host "---"
    $usages = Get-ChildItem -Path $SearchDir -Include $SourceExts -Recurse -File -ErrorAction SilentlyContinue | Where-Object {
        $exclude = $false
        foreach ($d in $ExcludeDirs) { if ($_.FullName -match "\\$d\\") { $exclude = $true; break } }
        -not $exclude
    } | Select-String -Pattern "\b$Target\b" -ErrorAction SilentlyContinue | Select-Object -First 30
    if ($usages) { $usages | ForEach-Object { Write-Host "$($_.Filename):$($_.LineNumber): $($_.Line.Trim())" } } else { Write-Host "   (none found)" }

    Write-Host "`n📦 Import/Require References (インポート参照):"
    Write-Host "---"
    $ImportRefPattern = "(import.*$Target|require.*$Target|from.*$Target)"
    $imports = Get-ChildItem -Path $SearchDir -Include $SourceExts -Recurse -File -ErrorAction SilentlyContinue | Where-Object {
        $exclude = $false
        foreach ($d in $ExcludeDirs) { if ($_.FullName -match "\\$d\\") { $exclude = $true; break } }
        -not $exclude
    } | Select-String -Pattern $ImportRefPattern -ErrorAction SilentlyContinue | Select-Object -First 20
    if ($imports) { $imports | ForEach-Object { Write-Host "$($_.Filename):$($_.LineNumber): $($_.Line.Trim())" } } else { Write-Host "   (none found)" }
}

Write-Host "`n============================================================"
Write-Host "✅ Trace complete."
