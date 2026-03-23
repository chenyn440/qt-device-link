#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build-release-macos"
DIST_DIR="${ROOT_DIR}/dist"
APP_NAME="DeviceLink"
VERSION="$(bash "${ROOT_DIR}/scripts/version.sh")"
DMG_NAME="${APP_NAME}-macOS-${VERSION}.dmg"
APPLE_SIGN_IDENTITY="${APPLE_SIGN_IDENTITY:-}"
APPLE_NOTARY_PROFILE="${APPLE_NOTARY_PROFILE:-}"

find_qt_tool() {
    local tool_name="$1"

    if command -v "${tool_name}" >/dev/null 2>&1; then
        command -v "${tool_name}"
        return 0
    fi

    if [[ -n "${Qt6_DIR:-}" ]]; then
        local qt6_bin
        qt6_bin="$(cd "${Qt6_DIR}/../../bin" 2>/dev/null && pwd || true)"
        if [[ -n "${qt6_bin}" && -x "${qt6_bin}/${tool_name}" ]]; then
            echo "${qt6_bin}/${tool_name}"
            return 0
        fi
    fi

    echo "missing Qt tool: ${tool_name}" >&2
    exit 1
}

rm -rf "${BUILD_DIR}"
mkdir -p "${DIST_DIR}"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" -j4
ctest --test-dir "${BUILD_DIR}" --output-on-failure

APP_PATH="${BUILD_DIR}/${APP_NAME}.app"
if [[ ! -d "${APP_PATH}" ]]; then
    echo "missing app bundle: ${APP_PATH}" >&2
    exit 1
fi

MACDEPLOYQT="$(find_qt_tool macdeployqt)"

"${MACDEPLOYQT}" "${APP_PATH}" -verbose=1

if [[ -n "${APPLE_SIGN_IDENTITY}" ]]; then
    codesign --force --deep --options runtime --sign "${APPLE_SIGN_IDENTITY}" "${APP_PATH}"
fi

"${MACDEPLOYQT}" "${APP_PATH}" -dmg -verbose=1

GENERATED_DMG="${BUILD_DIR}/${APP_NAME}.dmg"
if [[ ! -f "${GENERATED_DMG}" ]]; then
    echo "missing generated dmg: ${GENERATED_DMG}" >&2
    exit 1
fi

if [[ -n "${APPLE_SIGN_IDENTITY}" ]]; then
    codesign --force --sign "${APPLE_SIGN_IDENTITY}" "${GENERATED_DMG}"
fi

if [[ -n "${APPLE_NOTARY_PROFILE}" ]]; then
    xcrun notarytool submit "${GENERATED_DMG}" --keychain-profile "${APPLE_NOTARY_PROFILE}" --wait
    xcrun stapler staple "${GENERATED_DMG}"
fi

cp "${GENERATED_DMG}" "${DIST_DIR}/${DMG_NAME}"
echo "created ${DIST_DIR}/${DMG_NAME}"
