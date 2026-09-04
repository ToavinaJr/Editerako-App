#!/usr/bin/env bash
set -euo pipefail

# Run Editerako. Pass --build to compile first.
# Usage: ./scripts/run.sh [Debug|Release|Asan|Ubsan|Tsan] [--build]

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=_common.sh
. "${SCRIPT_DIR}/_common.sh"

config="Debug"
do_build=0
for arg in "$@"; do
    case "$arg" in
        --build) do_build=1 ;;
        Debug|debug|Release|release|Asan|asan|Ubsan|ubsan|Tsan|tsan) config="$(normalize_config "$arg")" ;;
        *)
            echo "Usage: $0 [Debug|Release|Asan|Ubsan|Tsan] [--build]" >&2
            exit 1
            ;;
    esac
done

cd "$project_root"

prefix="$(qt_prefix || true)"
if [[ -n "${prefix}" ]]; then
    export CMAKE_PREFIX_PATH="$prefix"
    export LD_LIBRARY_PATH="${prefix}/lib:${LD_LIBRARY_PATH:-}"
    export PATH="${prefix}/bin:${PATH}"
fi

if [[ "$do_build" -eq 1 ]]; then
    require_tools
    bash "${SCRIPT_DIR}/build.sh" "$config"
fi

exe="$(executable_path "$config")"
if [[ ! -f "$exe" ]]; then
    echo "Executable introuvable: $exe" >&2
    echo "Compilez d'abord:  ./scripts/build.sh $config" >&2
    echo "ou lancez:         ./scripts/run.sh $config --build" >&2
    exit 1
fi

echo "Starting $exe"
exec "$exe"
