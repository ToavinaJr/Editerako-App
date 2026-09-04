# Configure the Editerako CMake build (Ninja preset).
# Usage: .\scripts\configure.ps1 [-Config Debug|Release]

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

if (-not (Test-Path (Join-Path $root 'CMakeLists.txt'))) {
    throw "CMakeLists.txt introuvable a la racine: $root"
}
if (-not (Test-Path (Join-Path $root 'src\main.cpp'))) {
    throw "src\main.cpp introuvable - sources attendues sous src\."
}

$prefix = Get-QtPrefix
Initialize-BuildEnvironment -QtPrefix $prefix

$preset = Get-PresetName $Config
Write-Host "Project root : $root"
Write-Host "Preset       : $preset"
if ($prefix) {
    Write-Host "Qt prefix    : $prefix"
} else {
    Write-Host "Qt prefix    : (CMake default search path)"
}

$cmakeArgs = @('--preset', $preset)
if ($prefix) {
    $cmakeArgs += "-DCMAKE_PREFIX_PATH=$prefix"
}

& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) {
    throw "CMake configure a echoue (code $LASTEXITCODE)."
}

Write-Host "Configure OK -> $(Get-BuildDir $Config)"
