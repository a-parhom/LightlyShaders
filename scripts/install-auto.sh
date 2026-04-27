#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

PLASMA_VERSION_LINE="$(plasmashell --version 2>/dev/null || true)"
PLASMA_MAJOR=""

if [ -n "${PLASMA_VERSION_LINE}" ]; then
	PLASMA_MAJOR="$(printf '%s' "${PLASMA_VERSION_LINE}" | awk '{print $2}' | cut -d. -f1)"
fi

if [ "${PLASMA_MAJOR}" = "5" ]; then
	echo "[LightlyShaders] Detected Plasma 5.x. Running Qt5 installer."
	exec "${ROOT_DIR}/scripts/install-qt5.sh"
fi

if [ "${PLASMA_MAJOR}" = "6" ]; then
	echo "[LightlyShaders] Detected Plasma 6.x. Running Qt6 installer."
	exec "${ROOT_DIR}/scripts/install-qt6.sh"
fi

echo "[LightlyShaders] Could not detect Plasma major version from: '${PLASMA_VERSION_LINE}'"
echo "[LightlyShaders] Run one of these explicitly:"
echo "  scripts/install-qt5.sh"
echo "  scripts/install-qt6.sh"
exit 1
