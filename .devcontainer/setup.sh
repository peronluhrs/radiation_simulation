#!/usr/bin/env bash
set -euo pipefail

sudo apt-get update
sudo apt-get install -y \
  ninja-build pkg-config \
  libglm-dev libgl1-mesa-dev mesa-common-dev \
  qt6-base-dev qt6-base-dev-tools

# Configure + Build (Release)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_GUI=ON
cmake --build build -j
