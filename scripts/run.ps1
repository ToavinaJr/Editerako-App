# Run Editerako. Optionally build first.
# Usage: .\scripts\run.ps1 [-Config Debug|Release] [-Build]

[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string] $Config = 'Debug',
    [switch] $Build
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

. "$PSScriptRoot\_common.ps1"

$root = Get-ProjectRoot
Set-Location $root

$prefix = Get-QtPrefix
if ($prefix) {
    $env:PATH = "$(Join-Path $prefix 'bin');$env:PATH"
}

if ($Build) {
    Initialize-BuildEnvironment -QtPrefix $prefix
    & (Join-Path $PSScriptRoot 'build.ps1') -Config $Config
}

$exe = Get-ExecutablePath $Config
if (-not (Test-Path $exe)) {
    throw @"
Executable introuvable: $exe
Compilez d'abord:  .\scripts\build.ps1 -Config $Config
ou lancez:         .\scripts\run.ps1 -Build
"@
}

Write-Host "Starting $exe"
& $exe
