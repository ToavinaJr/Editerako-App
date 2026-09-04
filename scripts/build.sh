#!/usr/bin/env bash
set -euo pipefail

# Build Editerako. Configures first if the preset build directory is missing.
# Usage: ./scripts/build.sh [Debug|Release|Asan|Ubsan|Tsan]

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=_common.sh
. "${SCRIPT_DIR}/_common.sh"

config="$(normalize_config "${1:-Debug}")"
preset="$(preset_name "$config")"
prefix="$(qt_prefix || true)"

cd "$project_root"
require_tools

if [[ -n "${prefix}" ]]; then
    export CMAKE_PREFIX_PATH="$prefix"
fi

cache="$(build_dir "$config")/CMakeCache.txt"
if [[ ! -f "$cache" ]]; then
    echo "Build directory absent, configure..."
    bash "${SCRIPT_DIR}/configure.sh" "$config"
fi

echo "Building preset $preset ..."
cmake --build --preset "$preset"

exe="$(executable_path "$config")"
if [[ ! -x "$exe" && ! -f "$exe" ]]; then
    echo "Build termine mais l'executable est introuvable: $exe" >&2
    exit 1
fi

echo "Build OK -> $exe"
