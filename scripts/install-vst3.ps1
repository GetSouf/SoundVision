param(
    [string]$Source = "",
    [string]$Destination = "C:\Program Files\Common Files\VST3"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot

if ([string]::IsNullOrWhiteSpace($Source)) {
    $candidates = @(
        (Join-Path $Root "build\SoundVision_artefacts\Release\VST3\SoundVision.vst3"),
        (Join-Path $env:LOCALAPPDATA "Programs\Common\VST3\SoundVision.vst3")
    )
    $Source = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
}

if (-not $Source -or -not (Test-Path $Source)) {
    throw "SoundVision.vst3 not found. Build first: .\scripts\build.ps1"
}

if (-not (Test-Path $Destination)) {
    New-Item -ItemType Directory -Path $Destination -Force | Out-Null
}

# Remove stale duplicates that make FL show "SoundVision 2"
Get-ChildItem (Join-Path $Destination "SoundVision.vst3.bak_*") -ErrorAction SilentlyContinue | ForEach-Object {
    Remove-Item -LiteralPath $_.FullName -Recurse -Force -ErrorAction SilentlyContinue
    Write-Host "Removed stale: $($_.FullName)"
}
Get-ChildItem (Join-Path $Destination "SoundVision.vst3.old_*") -ErrorAction SilentlyContinue | ForEach-Object {
    Remove-Item -LiteralPath $_.FullName -Recurse -Force -ErrorAction SilentlyContinue
    Write-Host "Removed stale: $($_.FullName)"
}

$appDataCopy = Join-Path $env:LOCALAPPDATA "Programs\Common\VST3\SoundVision.vst3"
if ((Test-Path $appDataCopy) -and ($appDataCopy -ne $Source)) {
    Remove-Item -LiteralPath $appDataCopy -Recurse -Force -ErrorAction SilentlyContinue
    Write-Host "Removed duplicate: $appDataCopy"
}

$target = Join-Path $Destination "SoundVision.vst3"
if (Test-Path $target) {
    $bakName = "SoundVision.vst3.old_" + (Get-Date -Format "HHmmss")
    $bak = Join-Path $Destination $bakName
    try {
        Rename-Item -LiteralPath $target -NewName $bakName
        Remove-Item -LiteralPath $bak -Recurse -Force -ErrorAction SilentlyContinue
    } catch {
        throw "Could not replace installed plugin. Close FL Studio and retry. $($_.Exception.Message)"
    }
}

Copy-Item -LiteralPath $Source -Destination $Destination -Recurse -Force

Get-ChildItem (Join-Path $Destination "SoundVision.vst3.old_*") -ErrorAction SilentlyContinue | ForEach-Object {
    Remove-Item -LiteralPath $_.FullName -Recurse -Force -ErrorAction SilentlyContinue
}

$dll = Join-Path $target "Contents\x86_64-win\SoundVision.vst3"
$info = Join-Path $target "Contents\Resources\moduleinfo.json"
Write-Host "Installed: $target"
if (Test-Path $dll) { Write-Host "Binary:  $((Get-Item $dll).Length) bytes  $((Get-Item $dll).LastWriteTime)" }
if (Test-Path $info) { Select-String -Path $info -Pattern '"Version"' | Select-Object -First 1 | ForEach-Object { Write-Host $_.Line.Trim() } }
Write-Host "In FL: Manage plugins -> Rescan. Remove leftover 'SoundVision 2' entry if it remains."
