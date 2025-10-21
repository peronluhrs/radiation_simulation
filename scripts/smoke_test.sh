#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT/build/smoke"

echo "== Configuration Release =="
cmake -S "$ROOT" -B "$BUILD_DIR" -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_GUI=OFF

echo "== Compilation =="
cmake --build "$BUILD_DIR" --target RadiationSimConsole -j

echo "== Exécution RadiationSimConsole =="
LOG_FILE="$(mktemp)"
"$BUILD_DIR/RadiationSimConsole" | tee "$LOG_FILE"

echo "== Validation des détections =="
python3 - <<'PY' "$LOG_FILE"
import sys

log_path = sys.argv[1]
with open(log_path, 'r', encoding='utf-8') as fh:
    data = fh.read()

targets = {"Avant_Blindage", "Apres_Plomb", "Apres_Beton"}
counts = {}

for line in data.splitlines():
    parts = line.strip().split()
    if not parts:
        continue
    name = parts[0]
    if name in targets and len(parts) > 1 and parts[1].isdigit():
        counts[name] = int(parts[1])

missing = [name for name in ("Avant_Blindage", "Apres_Plomb", "Apres_Beton") if name not in counts]
if missing:
    sys.stderr.write(f"Capteurs manquants dans la sortie: {', '.join(missing)}\n")
    sys.exit(1)

avant = counts["Avant_Blindage"]
if avant <= 0:
    sys.stderr.write("Le capteur Avant_Blindage doit enregistrer des détections (> 0).\n")
    sys.exit(1)

if counts["Apres_Plomb"] > avant:
    sys.stderr.write("Apres_Plomb doit être inférieur ou égal à Avant_Blindage.\n")
    sys.exit(1)

if counts["Apres_Beton"] > avant:
    sys.stderr.write("Apres_Beton doit être inférieur ou égal à Avant_Blindage.\n")
    sys.exit(1)

print("[SMOKE] Vérifications réussies.")
PY
rm -f "$LOG_FILE"
