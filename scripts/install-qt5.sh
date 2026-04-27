#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
QT5_TAG="v2.2.1"
WORKTREE_DIR="${ROOT_DIR}/.worktrees/qt5-${QT5_TAG}"
BUILD_DIR="${WORKTREE_DIR}/build"

echo "[LightlyShaders] Preparing Qt5/KF5 (Plasma 5.27) worktree from ${QT5_TAG}"

mkdir -p "${ROOT_DIR}/.worktrees"

if [ ! -d "${WORKTREE_DIR}" ]; then
	git -C "${ROOT_DIR}" worktree add "${WORKTREE_DIR}" "${QT5_TAG}"
fi

set +o pipefail
LIBKWIN_PATH="$(ldconfig -p | grep -E 'libkwin\.so\.5 \(' | head -n1 | awk '{print $NF}')"
set -o pipefail
if [ -n "${LIBKWIN_PATH}" ]; then
	LIBKWIN_DIR="$(dirname "${LIBKWIN_PATH}")"
	if [ ! -e "${LIBKWIN_DIR}/libkwin.so" ]; then
		echo "[LightlyShaders] Creating compatibility symlink ${LIBKWIN_DIR}/libkwin.so -> libkwin.so.5"
		sudo ln -sf "${LIBKWIN_PATH}" "${LIBKWIN_DIR}/libkwin.so"
	fi
else
	echo "[LightlyShaders] Warning: libkwin.so.5 not found in ldconfig cache."
fi

rm -rf "${BUILD_DIR}"
cmake -S "${WORKTREE_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build "${BUILD_DIR}" -j"$(nproc)"
sudo cmake --install "${BUILD_DIR}"

echo "[LightlyShaders] Qt5 installation finished from ${QT5_TAG}."
