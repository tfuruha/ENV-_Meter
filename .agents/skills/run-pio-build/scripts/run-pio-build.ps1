param(
    [Parameter(ValueFromRemainingArguments=$true)]
    [string[]]$Args
)

$os = [System.Environment]::OSVersion.Platform
if ($os -eq "Win32NT") {
    $pio = "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe"
} else {
    $pio = "$env:HOME/.platformio/penv/bin/pio"
}

if (-Not (Test-Path $pio)) {
    Write-Error "PlatformIO CLI not found at: $pio"
    exit 1
}

Write-Host "Running PlatformIO: $pio run $($Args -join ' ')"
& $pio run $Args
