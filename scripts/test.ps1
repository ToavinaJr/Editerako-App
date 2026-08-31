# Build then run the Qt Test suite (ctest).
# Usage: .\scripts\test.ps1 [-Config Debug|Release]

[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string] $Config = 'Debug'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

. "$PSScriptRoot\_common.ps1"

$root = Get-ProjectRoot
Set-Location $root

$prefix = Get-QtPrefix
Initialize-BuildEnvironment -QtPrefix $prefix

& (Join-Path $PSScriptRoot 'build.ps1') -Config $Config

$preset = Get-PresetName $Config
Write-Host "Running tests (preset $preset) ..."
& ctest --preset $preset --output-on-failure
if ($LASTEXITCODE -ne 0) {
    throw "Tests echoues (code $LASTEXITCODE)."
}

Write-Host "Tests OK"
