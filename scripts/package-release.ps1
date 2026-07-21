param(
    [string]$Version = "",
    [string]$Source = ""
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot

if ([string]::IsNullOrWhiteSpace($Version)) {
    $cmake = Get-Content (Join-Path $Root "CMakeLists.txt") -Raw
    if ($cmake -match 'project\(SoundVision VERSION ([0-9.]+)') {
        $Version = $Matches[1]
    } else {
        throw "Could not read version from CMakeLists.txt"
    }
}

if ([string]::IsNullOrWhiteSpace($Source)) {
    $Source = Join-Path $Root "build\SoundVision_artefacts\Release\VST3\SoundVision.vst3"
}

if (-not (Test-Path $Source)) {
    throw "VST3 not found at $Source. Build first: .\scripts\build.ps1"
}

$dist = Join-Path $Root "dist"
New-Item -ItemType Directory -Path $dist -Force | Out-Null

$zipName = "SoundVision-v$Version-windows-x64.zip"
$zipPath = Join-Path $dist $zipName
if (Test-Path $zipPath) {
    Remove-Item -LiteralPath $zipPath -Force
}

$stage = Join-Path $dist "stage"
if (Test-Path $stage) {
    Remove-Item -LiteralPath $stage -Recurse -Force
}
New-Item -ItemType Directory -Path $stage -Force | Out-Null
Copy-Item -LiteralPath $Source -Destination (Join-Path $stage "SoundVision.vst3") -Recurse -Force

Compress-Archive -Path (Join-Path $stage "SoundVision.vst3") -DestinationPath $zipPath -Force
Remove-Item -LiteralPath $stage -Recurse -Force

Write-Host "Packaged: $zipPath"
Write-Host "Size:     $((Get-Item $zipPath).Length) bytes"
Write-Host "Upload this zip as a GitHub Release asset for v$Version"
