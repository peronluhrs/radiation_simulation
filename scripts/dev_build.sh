#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Dépendances (Ubuntu)
if command -v apt >/dev/null 2>&1; then
  sudo apt update
  sudo apt install -y build-essential cmake ninja-build \
    qt6-base-dev qt6-base-dev-tools libgl1-mesa-dev mesa-common-dev \
    libglm-dev
fi

rm -rf "$ROOT/build"
cmake -S "$ROOT" -B "$ROOT/build" -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_GUI=ON \
  -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt6
cmake --build "$ROOT/build" -j

echo "[OK] build terminé."
echo "Binaires:"
ls -lh "$ROOT/build/"RadiationSim*
