# Shared helpers for Editerako Windows scripts.
# Dot-source from the same directory:  . "$PSScriptRoot\_common.ps1"

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-ProjectRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
}

function Get-PresetName {
    param(
        [Parameter(Mandatory = $true)]
        [ValidateSet('Debug', 'Release', 'Asan', 'Ubsan', 'Tsan')]
        [string] $Config
    )
    return $Config.ToLowerInvariant()
}

function Get-BuildDir {
    param(
        [Parameter(Mandatory = $true)]
        [ValidateSet('Debug', 'Release', 'Asan', 'Ubsan', 'Tsan')]
        [string] $Config
    )
    return Join-Path (Get-ProjectRoot) "build\$(Get-PresetName $Config)"
}

function Get-ExecutablePath {
    param(
        [Parameter(Mandatory = $true)]
        [ValidateSet('Debug', 'Release', 'Asan', 'Ubsan', 'Tsan')]
        [string] $Config
    )
    return Join-Path (Get-BuildDir $Config) 'Editerako.exe'
}

function Get-FirstPathEntry {
    param([string] $Value)
    if ([string]::IsNullOrWhiteSpace($Value)) {
        return $null
    }
    return ($Value -split ';' | Where-Object { $_ -and $_.Trim() } | Select-Object -First 1).Trim()
}

function Get-QtPrefix {
    $fromEnv = Get-FirstPathEntry $env:CMAKE_PREFIX_PATH
    if ($fromEnv -and (Test-Path (Join-Path $fromEnv 'lib\cmake\Qt6'))) {
        return $fromEnv
    }

    if ($env:QTDIR -and (Test-Path (Join-Path $env:QTDIR 'lib\cmake\Qt6'))) {
        return $env:QTDIR
    }

    foreach ($cmdName in @('qtpaths', 'qmake')) {
        $cmd = Get-Command $cmdName -ErrorAction SilentlyContinue
        if (-not $cmd) { continue }
        $prefix = $null
        if ($cmdName -eq 'qtpaths') {
            $prefix = (& $cmd.Source --install-prefix 2>$null | Select-Object -First 1)
        } else {
            $prefix = (& $cmd.Source -query QT_INSTALL_PREFIX 2>$null | Select-Object -First 1)
        }
        if ($prefix -and (Test-Path (Join-Path $prefix 'lib\cmake\Qt6'))) {
            return $prefix.Trim()
        }
    }

    # Official Qt Online Installer layout (not a user home path).
    $qtRoot = 'C:\Qt'
    if (Test-Path $qtRoot) {
        $candidates = Get-ChildItem -Path $qtRoot -Directory -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -match '^\d+\.\d+' } |
            Sort-Object Name -Descending
        foreach ($ver in $candidates) {
            foreach ($kit in @('mingw_64', 'msvc2022_64', 'msvc2019_64')) {
                $prefix = Join-Path $ver.FullName $kit
                if (Test-Path (Join-Path $prefix 'lib\cmake\Qt6')) {
                    return $prefix
                }
            }
        }
    }

    return $null
}

function Get-QtRootFromPrefix {
    param([string] $Prefix)
    if (-not $Prefix) { return $null }
    $kitDir = Split-Path -Parent $Prefix
    $qtRoot = Split-Path -Parent $kitDir
    if ($qtRoot -and (Test-Path (Join-Path $qtRoot 'Tools'))) {
        return $qtRoot
    }
    return $null
}

function Add-ToPathIfExists {
    param([string] $Directory)
    if ($Directory -and (Test-Path $Directory)) {
        $env:PATH = "$Directory;$env:PATH"
    }
}

function Initialize-BuildEnvironment {
    param([string] $QtPrefix)

    if ($QtPrefix) {
        Add-ToPathIfExists (Join-Path $QtPrefix 'bin')
        $env:CMAKE_PREFIX_PATH = $QtPrefix
        $qtRoot = Get-QtRootFromPrefix $QtPrefix
        if ($qtRoot) {
            Add-ToPathIfExists (Join-Path $qtRoot 'Tools\CMake_64\bin')
            Add-ToPathIfExists (Join-Path $qtRoot 'Tools\Ninja')
            $mingw = Get-ChildItem (Join-Path $qtRoot 'Tools') -Directory -ErrorAction SilentlyContinue |
                Where-Object { $_.Name -like 'mingw*' } |
                Sort-Object Name -Descending |
                Select-Object -First 1
            if ($mingw) {
                Add-ToPathIfExists (Join-Path $mingw.FullName 'bin')
            }
        }
    }

    $missing = @()
    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) { $missing += 'cmake' }
    if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) { $missing += 'ninja' }
    $hasCxx = (Get-Command g++ -ErrorAction SilentlyContinue) -or (Get-Command cl -ErrorAction SilentlyContinue) -or (Get-Command clang++ -ErrorAction SilentlyContinue)
    if (-not $hasCxx) { $missing += 'C++ compiler (g++, cl, or clang++)' }

    if ($missing.Count -gt 0) {
        throw @"
Outils introuvables: $($missing -join ', ').
Installez CMake, Ninja et un compilateur C++20, puis soit:
  - ajoutez-les au PATH
  - soit definissez CMAKE_PREFIX_PATH ou QTDIR vers le kit Qt (ex. ...\6.x.y\mingw_64)
Le module Qt PDF (PdfWidgets) et Qt SVG (SvgWidgets) doivent etre installes.
"@
    }
}
