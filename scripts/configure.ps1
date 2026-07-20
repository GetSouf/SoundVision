param(
    [string]$JucePath = "C:/dev/JUCE",
    [string]$BuildDir = "build",
    [string]$Generator = ""
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
if (-not $cmake) {
    throw "cmake.exe not found. Install Visual Studio C++ workload or CMake."
}

if (-not (Test-Path "$JucePath/CMakeLists.txt")) {
    throw "JUCE not found at $JucePath"
}

if ([string]::IsNullOrWhiteSpace($Generator)) {
    if (Test-Path "${env:ProgramFiles}\Microsoft Visual Studio\18\Community") {
        $Generator = "Visual Studio 18 2026"
    }
    else {
        $Generator = "Visual Studio 17 2022"
    }
}

Write-Host "Using CMake: $cmake"
Write-Host "Generator:   $Generator"
Write-Host "JUCE:        $JucePath"

& $cmake -S $Root -B (Join-Path $Root $BuildDir) -G $Generator -A x64 "-DJUCE_PATH=$JucePath"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Configure OK. Open $(Join-Path $Root $BuildDir)\SoundVision.sln or run .\scripts\build.ps1"
