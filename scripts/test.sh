#!/usr/bin/env bash
set -euo pipefail

# Build then run the Qt Test suite (ctest).
# Usage: ./scripts/test.sh [Debug|Release|Asan|Ubsan|Tsan]

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

"${SCRIPT_DIR}/build.sh" "$config"

echo "Running tests (preset $preset) ..."
ctest --preset "$preset" --output-on-failure

echo "Tests OK"
