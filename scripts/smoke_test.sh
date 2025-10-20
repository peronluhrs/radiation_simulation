#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Build si manquant
[ -x "$ROOT/build/RadiationSimConsole" ] || "$ROOT/scripts/dev_build.sh"

echo "== CLI --version =="
"$ROOT/build/RadiationSimConsole" --version || true

echo "== Vérif GUI binaire =="
test -x "$ROOT/build/RadiationSimGUI" && echo "GUI présent." || { echo "GUI absent"; exit 1; }

echo "[SMOKE] OK."
