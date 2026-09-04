#!/usr/bin/env bash
set -euo pipefail
# Ubuntu packages needed to run Qt Widgets tests (offscreen) and to link Qt from aqtinstall.
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
    ninja-build \
    libgl1-mesa-dev \
    libfontconfig1 \
    libxkbcommon-dev \
    libxkbcommon-x11-dev \
    libxcb-cursor0 \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-randr0 \
    libxcb-render-util0 \
    libxcb-shape0 \
    libxcb-xinerama0 \
    libxcb-xfixes0 \
    libegl1 \
    libdbus-1-3 \
    libicu-dev
