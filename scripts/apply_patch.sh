#!/usr/bin/env bash
set -euo pipefail

TMP_PATCH=$(mktemp)
trap 'rm -f "$TMP_PATCH"' EXIT
cat > "$TMP_PATCH"
git apply "$TMP_PATCH"
