param(
    [int]$Depth = 4,
    [string]$TargetDir = "."
)

if ($TargetDir -eq "--help" -or $TargetDir -eq "-h") {
    Write-Host "Usage: .\index-structure.ps1 [-Depth N] [-TargetDir PATH]"
    exit
}

# --- プロジェクトルートの自動解決 ---
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$ProjectRoot = Resolve-Path (Join-Path $ScriptDir "..\..\..\..")
Set-Location $ProjectRoot

Write-Host "📁 Project Structure Index"
Write-Host "   Root: $ProjectRoot"
Write-Host "   Target: $TargetDir"
Write-Host "   Depth: $Depth"
Write-Host "============================================================"

# --- 除外パターン ---
$ExcludePatterns = @('node_modules', '\.git', '\.DS_Store', 'dist', 'build', '__pycache__', '\.next', '\.venv', 'target')

# --- ツリー出力 ---
$Files = Get-ChildItem -Path $TargetDir -Recurse -Depth $Depth -Force | Where-Object {
    $path = $_.FullName
    $exclude = $false
    foreach ($pattern in $ExcludePatterns) {
        if ($path -match "\\$pattern\\|\\$pattern$") { $exclude = $true; break }
    }
    -not $exclude
}

foreach ($Item in $Files) {
    # インデントの計算（深さに応じて）
    $relativePath = $Item.FullName.Substring((Resolve-Path $TargetDir).Path.Length).Trim('\')
    if (-not $relativePath) { continue }
    $indentCount = ($relativePath.Split('\').Count - 1) * 2
    $indent = " " * $indentCount

    if ($Item.PSIsContainer) {
        $count = (Get-ChildItem -Path $Item.FullName -Force -ErrorAction SilentlyContinue).Count
        Write-Host "${indent}📂 $($Item.Name)/ ($count items)"
    } else {
        $size = $Item.Length
        Write-Host "${indent}📄 $($Item.Name) (${size}B)"
    }
}

# --- サマリー統計 ---
Write-Host ""
Write-Host "============================================================"
Write-Host "📊 Summary:"

$allFiles = $Files | Where-Object { -not $_.PSIsContainer }
$allDirs = $Files | Where-Object { $_.PSIsContainer }
Write-Host "   Files: $($allFiles.Count) | Directories: $($allDirs.Count)"
Write-Host "   File types:"

$allFiles | Group-Object Extension | Sort-Object Count -Descending | Select-Object -First 10 | ForEach-Object {
    $ext = if ($_.Name) { $_.Name } else { "none" }
    Write-Host "     ${ext}: $($_.Count)"
}
