#!/usr/bin/env bash
set -euo pipefail

# Run clang-tidy on src/ and tests/ using compile_commands.json from a preset.
# Usage: ./scripts/tidy.sh [debug]

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=_common.sh
. "${SCRIPT_DIR}/_common.sh"

config="$(normalize_config "${1:-Debug}")"
build="$(build_dir "$config")"
compile_db="${build}/compile_commands.json"

if [[ ! -f "$compile_db" ]]; then
    echo "compile_commands.json introuvable: $compile_db" >&2
    echo "Configurez d'abord: ./scripts/configure.sh $config" >&2
    exit 1
fi

if command -v run-clang-tidy >/dev/null 2>&1; then
    tidy=(run-clang-tidy)
elif command -v run-clang-tidy-18 >/dev/null 2>&1; then
    tidy=(run-clang-tidy-18)
elif command -v run-clang-tidy-19 >/dev/null 2>&1; then
    tidy=(run-clang-tidy-19)
else
    echo "run-clang-tidy introuvable. Installez clang-tidy." >&2
    exit 1
fi

cd "$project_root"
"${tidy[@]}" -p "$build" -header-filter='.*/src/.*' 'src/.*' 'tests/.*'
