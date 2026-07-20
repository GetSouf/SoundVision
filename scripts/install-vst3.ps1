# Run this script as Administrator to install SoundVision into FL Studio's default VST3 folder.
# Right-click PowerShell -> Run as administrator, then:
#   Set-ExecutionPolicy -Scope Process Bypass
#   .\scripts\install-vst3.ps1

param(
    [string]$Source = "",
    [string]$Destination = "C:\Program Files\Common Files\VST3"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot

if ([string]::IsNullOrWhiteSpace($Source)) {
    $candidates = @(
        (Join-Path $env:LOCALAPPDATA "Programs\Common\VST3\SoundVision.vst3"),
        (Join-Path $Root "build\SoundVision_artefacts\Release\VST3\SoundVision.vst3")
    )
    $Source = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
}

if (-not $Source -or -not (Test-Path $Source)) {
    throw "SoundVision.vst3 not found. Build first: .\scripts\build.ps1"
}

if (-not (Test-Path $Destination)) {
    New-Item -ItemType Directory -Path $Destination -Force | Out-Null
}

$target = Join-Path $Destination "SoundVision.vst3"
if (Test-Path $target) {
    Remove-Item -LiteralPath $target -Recurse -Force
}

Copy-Item -LiteralPath $Source -Destination $Destination -Recurse -Force
Write-Host "Installed: $target"
Write-Host "In FL Studio: Options -> Manage plugins -> Find more plugins / Verify plugins"
