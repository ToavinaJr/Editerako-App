#!/usr/bin/env bash
set -euo pipefail

# Run clang-tidy on application sources under src/ (not tree-sitter, not tests).
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

regex_escape() {
    printf '%s' "$1" | sed -e 's/[][(){}.^$+?\\|]/\\&/g'
}

# Absolute <root>/src/ so tree-sitter/**/src and build/**/src autogen are skipped.
src_dir="${project_root}/src"
src_regex="$(regex_escape "${src_dir}")/"

cd "$project_root"
echo "clang-tidy on ${src_dir} (preset $(preset_name "$config")) ..."
"${tidy[@]}" -p "$build" -header-filter="${src_regex}" "${src_regex}"
