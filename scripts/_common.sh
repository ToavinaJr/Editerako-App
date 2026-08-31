#!/usr/bin/env bash
# Shared helpers for Editerako Linux scripts.
# shellcheck shell=bash
# Usage: # shellcheck source=_common.sh
#        . "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/_common.sh"

set -euo pipefail

scripts_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd "${scripts_dir}/.." && pwd)"

preset_name() {
    local config="${1:-Debug}"
    echo "${config,,}"
}

build_dir() {
    local config="${1:-Debug}"
    echo "${project_root}/build/$(preset_name "$config")"
}

executable_path() {
    local config="${1:-Debug}"
    echo "$(build_dir "$config")/Editerako"
}

first_path_entry() {
    local value="${1:-}"
    if [[ -z "$value" ]]; then
        return 0
    fi
    echo "${value%%:*}"
}

qt_prefix() {
    local from_env
    from_env="$(first_path_entry "${CMAKE_PREFIX_PATH:-}")"
    if [[ -n "$from_env" && -d "$from_env/lib/cmake/Qt6" ]]; then
        echo "$from_env"
        return 0
    fi
    if [[ -n "${QTDIR:-}" && -d "${QTDIR}/lib/cmake/Qt6" ]]; then
        echo "$QTDIR"
        return 0
    fi
    if command -v qtpaths >/dev/null 2>&1; then
        local prefix
        prefix="$(qtpaths --install-prefix 2>/dev/null | head -n 1 || true)"
        if [[ -n "$prefix" && -d "$prefix/lib/cmake/Qt6" ]]; then
            echo "$prefix"
            return 0
        fi
    fi
    if command -v qmake >/dev/null 2>&1; then
        local prefix
        prefix="$(qmake -query QT_INSTALL_PREFIX 2>/dev/null | head -n 1 || true)"
        if [[ -n "$prefix" && -d "$prefix/lib/cmake/Qt6" ]]; then
            echo "$prefix"
            return 0
        fi
    fi
    return 0
}

require_tools() {
    local missing=()
    command -v cmake >/dev/null 2>&1 || missing+=("cmake")
    command -v ninja >/dev/null 2>&1 || missing+=("ninja")
    if ! command -v g++ >/dev/null 2>&1 && ! command -v clang++ >/dev/null 2>&1; then
        missing+=("C++ compiler (g++ or clang++)")
    fi
    if ((${#missing[@]} > 0)); then
        echo "Outils introuvables: ${missing[*]}" >&2
        echo "Installez CMake (>= 3.21), Ninja, un compilateur C++20, Qt 6 (Widgets, Network, Sql, Concurrent, PdfWidgets)." >&2
        echo "Definissez CMAKE_PREFIX_PATH ou QTDIR si CMake ne trouve pas Qt." >&2
        exit 1
    fi
}

normalize_config() {
    local config="${1:-Debug}"
    case "${config,,}" in
        debug) echo "Debug" ;;
        release) echo "Release" ;;
        *)
            echo "Configuration invalide: $config (Debug ou Release)" >&2
            exit 1
            ;;
    esac
}
