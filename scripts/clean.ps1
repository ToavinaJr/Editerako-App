# Remove preset build directories.
# Usage: .\scripts\clean.ps1 [-Config Debug|Release] [-All]
#   -Config  removes only build/debug or build/release
#   -All     removes the entire build/ directory (including Qt Creator kits)

[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string] $Config = 'Debug',
    [switch] $All
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

. "$PSScriptRoot\_common.ps1"

$root = Get-ProjectRoot

if ($All) {
    $buildRoot = Join-Path $root 'build'
    if (Test-Path $buildRoot) {
        Remove-Item -LiteralPath $buildRoot -Recurse -Force
        Write-Host "Removed $buildRoot"
    } else {
        Write-Host "Nothing to clean: $buildRoot"
    }
    return
}

$dir = Get-BuildDir $Config
if (Test-Path $dir) {
    Remove-Item -LiteralPath $dir -Recurse -Force
    Write-Host "Removed $dir"
} else {
    Write-Host "Nothing to clean: $dir"
}
