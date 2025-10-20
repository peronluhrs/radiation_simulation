#!/usr/bin/env bash
set -euo pipefail

# 0) Déps (NOP si déjà installées)
if ! command -v ninja >/dev/null 2>&1; then
  sudo apt-get update
  sudo apt-get install -y ninja-build
fi
sudo apt-get update
sudo apt-get install -y cmake pkg-config \
  qt6-base-dev qt6-base-dev-tools libglm-dev \
  libgl1-mesa-dev mesa-common-dev

# 1) Configure + build (Release)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_GUI=ON \
  -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt6
cmake --build build -j

# 2) Smoke test CLI
./build/RadiationSimConsole --version || true
./build/RadiationSimConsole || true

echo "✅ dev_build: OK"
