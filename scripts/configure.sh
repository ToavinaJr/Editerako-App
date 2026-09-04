#!/usr/bin/env bash
set -euo pipefail

# Configure the Editerako CMake build (Ninja preset).
# Usage: ./scripts/configure.sh [Debug|Release|Asan|Ubsan|Tsan]

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=_common.sh
. "${SCRIPT_DIR}/_common.sh"

config="$(normalize_config "${1:-Debug}")"
preset="$(preset_name "$config")"
prefix="$(qt_prefix || true)"

cd "$project_root"
require_tools

if [[ ! -f "$project_root/CMakeLists.txt" ]]; then
    echo "CMakeLists.txt introuvable a la racine: $project_root" >&2
    exit 1
fi
if [[ ! -f "$project_root/src/main.cpp" ]]; then
    echo "src/main.cpp introuvable - sources attendues sous src/." >&2
    exit 1
fi

echo "Project root : $project_root"
echo "Preset       : $preset"
if [[ -n "${prefix}" ]]; then
    echo "Qt prefix    : $prefix"
    export CMAKE_PREFIX_PATH="$prefix"
else
    echo "Qt prefix    : (CMake default search path)"
fi

if [[ -n "${prefix}" ]]; then
    cmake --preset "$preset" "-DCMAKE_PREFIX_PATH=${prefix}"
else
    cmake --preset "$preset"
fi

echo "Configure OK -> $(build_dir "$config")"
