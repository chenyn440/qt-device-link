#!/usr/bin/env bash
set -euo pipefail

if [[ $# -gt 1 ]]; then
    echo "usage: $0 [fallback-version]" >&2
    exit 1
fi

fallback_version="${1:-0.1.0}"

if [[ -n "${GITHUB_REF_NAME:-}" ]]; then
    echo "${GITHUB_REF_NAME#v}"
    exit 0
fi

if git describe --tags --exact-match >/dev/null 2>&1; then
    echo "$(git describe --tags --exact-match | sed 's/^v//')"
    exit 0
fi

echo "${fallback_version}"
