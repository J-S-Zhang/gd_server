#!/bin/bash
set -e
cd "$(dirname "$0")/.."
mkdir -p logs
PORT="${1:-9001}"
exec ./build/guandan_server "$PORT"
