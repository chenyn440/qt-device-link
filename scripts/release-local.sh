#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [[ $# -ne 1 ]]; then
    echo "usage: $0 <macos|windows>" >&2
    exit 1
fi

case "$1" in
    macos)
        exec bash "${ROOT_DIR}/scripts/package-macos.sh"
        ;;
    windows)
        if [[ "${OS:-}" == "Windows_NT" ]]; then
            exec powershell -ExecutionPolicy Bypass -File "${ROOT_DIR}/scripts/package-windows.ps1"
        fi
        echo "windows packaging must be run on Windows or via GitHub Actions" >&2
        exit 1
        ;;
    *)
        echo "unsupported target: $1" >&2
        exit 1
        ;;
esac
