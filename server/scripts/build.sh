#!/bin/bash
set -e
cd "$(dirname "$0")/.."
mkdir -p build
cd build
cmake -DGUANDAN_ENABLE_BOOST=ON ..
cmake --build . --config Release -j$(nproc)
echo "Build complete: build/guandan_server"
