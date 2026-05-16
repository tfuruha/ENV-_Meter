# プロジェクト構造認識 (Indexing Awareness) 検証スクリプト
# ドキュメント (README.md) と実ファイル (.agent/**/*) の整合性をチェックします。

# プロジェクトルートへ移動（スクリプトがどこから呼ばれても動作するように）
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$ProjectRoot = Resolve-Path (Join-Path $ScriptDir "..\..\..\..")
Set-Location $ProjectRoot

Write-Host "🔍 Verifying consistency between README.md and .agent files..."
Write-Host "   Project root: $ProjectRoot"
Write-Host "============================================================"

$ReadmeFile = "README.md"
$RulesDir = ".agent\rules"
$WorkflowsDir = ".agent\workflows"
$SkillsDir = ".agent\skills"
$ExitCode = 0

# README.md の存在チェック
if (-not (Test-Path $ReadmeFile)) {
    Write-Host "❌ README.md not found at project root."
    exit 1
}

$ReadmeContent = Get-Content $ReadmeFile -Raw -ErrorAction SilentlyContinue

# 1. Rules Check
Write-Host "Checking Rules..."
$RuleFiles = Get-ChildItem -Path $RulesDir -Filter "*.md" -ErrorAction SilentlyContinue
if (-not $RuleFiles) {
    Write-Host "⏭️  No rule files found in $RulesDir"
} else {
    foreach ($file in $RuleFiles) {
        if ($ReadmeContent -match [regex]::Escape($file.Name)) {
            Write-Host "✅ Found: $($file.Name)"
        } else {
            Write-Host "❌ Missing in README: $($file.Name)"
            $ExitCode = 1
        }
    }
}

# 2. Workflows Check
Write-Host "Checking Workflows..."
$WorkflowFiles = Get-ChildItem -Path $WorkflowsDir -Filter "*.md" -ErrorAction SilentlyContinue
if (-not $WorkflowFiles) {
    Write-Host "⏭️  No workflow files found in $WorkflowsDir"
} else {
    foreach ($file in $WorkflowFiles) {
        $commandName = "/" + $file.BaseName
        if ($ReadmeContent -match [regex]::Escape($commandName)) {
            Write-Host "✅ Found: $commandName"
        } else {
            Write-Host "❌ Missing in README: $commandName"
            $ExitCode = 1
        }
    }
}

# 3. Skills Check
Write-Host "Checking Skills..."
$SkillDirs = Get-ChildItem -Path $SkillsDir -Directory -ErrorAction SilentlyContinue
if (-not $SkillDirs) {
    Write-Host "⏭️  No skill directories found in $SkillsDir"
} else {
    foreach ($dir in $SkillDirs) {
        if ($ReadmeContent -match [regex]::Escape($dir.Name)) {
            Write-Host "✅ Found: $($dir.Name)"
        } else {
            Write-Host "❌ Missing in README: $($dir.Name)"
            $ExitCode = 1
        }
    }
}

Write-Host "============================================================"
if ($ExitCode -eq 0) {
    Write-Host "🎉 All checks passed! Project is well-indexed."
} else {
    Write-Host "⚠️  Some inconsistencies found. Please update README.md."
}

exit $ExitCode
