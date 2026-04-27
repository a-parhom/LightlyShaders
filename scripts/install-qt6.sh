#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build-qt6"

echo "[LightlyShaders] Building Qt6/KF6 (Plasma 6.x)"

if ! command -v qtpaths6 >/dev/null 2>&1; then
	echo "[LightlyShaders] Missing tool: qtpaths6"
	echo "[LightlyShaders] On Ubuntu/Debian, install at least:"
	echo "  sudo apt install qt6-base-dev qt6-base-dev-tools extra-cmake-modules"
	exit 1
fi

if ! pkg-config --exists Qt6Core; then
	echo "[LightlyShaders] Qt6 development files were not found (Qt6Core.pc missing)."
	echo "[LightlyShaders] Install Qt6/KF6 development packages before building this branch."
	echo "[LightlyShaders] Example (Ubuntu/Debian):"
	echo "  sudo apt install qt6-base-dev qt6-declarative-dev extra-cmake-modules libkf6config-dev libkf6coreaddons-dev libkf6windowsystem-dev kwin-dev"
	exit 1
fi

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build "${BUILD_DIR}" -j"$(nproc)"
sudo cmake --install "${BUILD_DIR}"

echo "[LightlyShaders] Qt6 installation finished."
