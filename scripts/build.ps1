param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo")]
    [string]$Config = "Release",
    [string]$BuildDir = "build",
    [string]$Target = "SoundVision_VST3"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
Set-Location $Root

$cmakeCandidates = @(
    "${env:ProgramFiles}\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
    "${env:ProgramFiles}\CMake\bin\cmake.exe"
)

$cmake = $cmakeCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $cmake) { throw "cmake.exe not found" }

if (-not (Test-Path (Join-Path $Root $BuildDir))) {
    & (Join-Path $PSScriptRoot "configure.ps1") -BuildDir $BuildDir
}

Write-Host "Building $Target ($Config)..."
& $cmake --build (Join-Path $Root $BuildDir) --config $Config --target $Target
exit $LASTEXITCODE
