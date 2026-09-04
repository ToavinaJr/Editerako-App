#!/usr/bin/env bash
set -euo pipefail

# Install a relocatable prefix, then a .tar.gz and (Linux) an AppImage when
# linuxdeploy is available or can be downloaded.
# Usage: ./scripts/package.sh [Release]

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=_common.sh
. "${SCRIPT_DIR}/_common.sh"

config="$(normalize_config "${1:-Release}")"
if [[ "$config" != "Release" ]]; then
    echo "Le packaging n'est prevu que pour Release (recu: $config)." >&2
    exit 1
fi

cd "$project_root"
prefix="$(qt_prefix || true)"
if [[ -n "${prefix}" ]]; then
    export CMAKE_PREFIX_PATH="$prefix"
    export PATH="${prefix}/bin:${PATH}"
    export LD_LIBRARY_PATH="${prefix}/lib:${LD_LIBRARY_PATH:-}"
fi

bash "${SCRIPT_DIR}/build.sh" "$config"

build_dir="$(build_dir "$config")"
version="0.1.0"
if [[ -f "${build_dir}/CMakeCache.txt" ]]; then
    version="$(sed -n 's/^CMAKE_PROJECT_VERSION:STATIC=//p' "${build_dir}/CMakeCache.txt" | head -n 1)"
    version="${version:-0.1.0}"
fi

os="$(uname -s)"
arch="$(uname -m)"
dist_root="${project_root}/dist"
mkdir -p "$dist_root"
stage="${dist_root}/Editerako-${version}-${os}-${arch}"
rm -rf "$stage"
mkdir -p "$stage"

echo "Installing to $stage ..."
cmake --install "$build_dir" --prefix "$stage"

tarball="${dist_root}/Editerako-${version}-${os}-${arch}.tar.gz"
tar -C "$dist_root" -czf "$tarball" "$(basename "$stage")"
echo "Archive OK -> $tarball"

if [[ "$os" != "Linux" ]]; then
    exit 0
fi

appdir="${dist_root}/Editerako.AppDir"
rm -rf "$appdir"
mkdir -p "$appdir"
cmake --install "$build_dir" --prefix "${appdir}/usr"

deploy="${LINUXDEPLOY:-}"
plugin="${LINUXDEPLOY_PLUGIN_QT:-}"
if [[ -z "$deploy" ]]; then
    tmp="${dist_root}/_linuxdeploy"
    mkdir -p "$tmp"
    deploy="${tmp}/linuxdeploy-x86_64.AppImage"
    plugin="${tmp}/linuxdeploy-plugin-qt-x86_64.AppImage"
    if [[ ! -x "$deploy" ]]; then
        echo "Telechargement de linuxdeploy ..."
        curl -L --fail -o "$deploy" \
            "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
        curl -L --fail -o "$plugin" \
            "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
        chmod +x "$deploy" "$plugin"
    fi
fi

if [[ -x "$deploy" ]]; then
    export APPIMAGE_EXTRACT_AND_RUN=1
    export QMAKE="${prefix:-}/bin/qmake"
    if [[ ! -x "$QMAKE" ]]; then
        QMAKE="$(command -v qmake6 || command -v qmake || true)"
        export QMAKE
    fi
    (
        cd "$dist_root"
        if ! "$deploy" --appdir "$appdir" --plugin qt --output appimage; then
            echo "AppImage echoue — le tarball reste disponible."
        else
            echo "AppImage OK -> ${dist_root}/Editerako*.AppImage"
        fi
    )
else
    echo "linuxdeploy introuvable — AppImage ignore (tarball disponible)."
fi
