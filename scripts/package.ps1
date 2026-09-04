# Install a portable Windows tree (Qt DLLs via the CMake deploy script) and zip it.
# Usage: .\scripts\package.ps1 [-Config Release]

[CmdletBinding()]
param(
    [ValidateSet('Release')]
    [string] $Config = 'Release'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

. "$PSScriptRoot\_common.ps1"

$root = Get-ProjectRoot
Set-Location $root

$prefix = Get-QtPrefix
Initialize-BuildEnvironment -QtPrefix $prefix
& (Join-Path $PSScriptRoot 'build.ps1') -Config $Config

$buildDir = Get-BuildDir $Config
$version = '0.1.0'
$cache = Join-Path $buildDir 'CMakeCache.txt'
if (Test-Path $cache) {
    $line = Select-String -Path $cache -Pattern '^CMAKE_PROJECT_VERSION:STATIC=(.+)$' | Select-Object -First 1
    if ($line) {
        $version = $line.Matches.Groups[1].Value
    }
}

$distRoot = Join-Path $root 'dist'
$stage = Join-Path $distRoot "Editerako-$version-win64"
if (Test-Path $stage) {
    Remove-Item -LiteralPath $stage -Recurse -Force
}
New-Item -ItemType Directory -Path $stage | Out-Null

Write-Host "Installing to $stage ..."
& cmake --install $buildDir --prefix $stage
if ($LASTEXITCODE -ne 0) {
    throw "cmake --install a echoue (code $LASTEXITCODE)."
}

$zip = Join-Path $distRoot "Editerako-$version-win64.zip"
if (Test-Path $zip) {
    Remove-Item -LiteralPath $zip -Force
}
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip
Write-Host "Package OK -> $zip"
