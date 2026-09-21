# Build with MSYS2 MinGW-w64 (UCRT64). Do not use the MSYS posix gcc.
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

$ucrt = "C:\msys64\ucrt64\bin"
if (-not (Test-Path "$ucrt\g++.exe")) {
    throw "MSYS2 UCRT64 g++ not found at $ucrt.`nInstall: pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make"
}

$env:PATH = "$ucrt;" + $env:PATH
$triple = & "$ucrt\g++.exe" -dumpmachine
if ($triple -notmatch "mingw") {
    throw "g++ target is '$triple'. Use UCRT64/mingw-w64, not msys gcc."
}

if (-not (Test-Path "third_party\imgui\imgui.cpp")) {
    git clone --depth 1 --branch v1.91.8 https://github.com/ocornut/imgui.git third_party/imgui
}

& "$ucrt\mingw32-make.exe" -j $env:NUMBER_OF_PROCESSORS
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Host "Built: $root\build-msys\NetProxyManager.exe"
