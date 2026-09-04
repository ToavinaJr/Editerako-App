# Build Editerako. Configures first if the preset build directory is missing.
# Usage: .\scripts\build.ps1 [-Config Debug|Release]

[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release', 'Asan', 'Ubsan', 'Tsan')]
    [string] $Config = 'Debug'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

. "$PSScriptRoot\_common.ps1"

$root = Get-ProjectRoot
Set-Location $root

$prefix = Get-QtPrefix
Initialize-BuildEnvironment -QtPrefix $prefix

$preset = Get-PresetName $Config
$buildDir = Get-BuildDir $Config

if (-not (Test-Path (Join-Path $buildDir 'CMakeCache.txt'))) {
    Write-Host "Build directory absent, configure..."
    & (Join-Path $PSScriptRoot 'configure.ps1') -Config $Config
}

Write-Host "Building preset $preset ..."
& cmake --build --preset $preset
if ($LASTEXITCODE -ne 0) {
    throw "CMake build a echoue (code $LASTEXITCODE)."
}

$exe = Get-ExecutablePath $Config
if (-not (Test-Path $exe)) {
    throw "Build termine mais l'executable est introuvable: $exe"
}

Write-Host "Build OK -> $exe"
