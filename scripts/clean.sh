#!/usr/bin/env bash
set -euo pipefail

# Remove preset build directories.
# Usage: ./scripts/clean.sh [Debug|Release|Asan|Ubsan|Tsan]
#        ./scripts/clean.sh --all

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=_common.sh
. "${SCRIPT_DIR}/_common.sh"

if [[ "${1:-}" == "--all" ]]; then
    build_root="${project_root}/build"
    if [[ -d "$build_root" ]]; then
        rm -rf "$build_root"
        echo "Removed $build_root"
    else
        echo "Nothing to clean: $build_root"
    fi
    exit 0
fi

config="$(normalize_config "${1:-Debug}")"
dir="$(build_dir "$config")"
if [[ -d "$dir" ]]; then
    rm -rf "$dir"
    echo "Removed $dir"
else
    echo "Nothing to clean: $dir"
fi
