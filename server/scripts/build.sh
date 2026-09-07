#!/bin/bash
set -e
cd "$(dirname "$0")/.."
mkdir -p build logs
cd build
cmake ..
cmake --build . -j"$(nproc)"
echo "Build complete: $(pwd)/guandan_server"
