#!/bin/bash
# 方案 A（Nginx 反代）：对外 80，本机 8080 + 9001
# 用法：./scripts/start.sh [ws_port] [http_port] [db_path]
set -e
cd "$(dirname "$0")/.."
mkdir -p logs

WS_PORT="${1:-9001}"
HTTP_PORT="${2:-8080}"
DB_PATH="${3:-./server/data/users.json}"

echo "启动 guandan_server WS=$WS_PORT HTTP=$HTTP_PORT DB=$DB_PATH"
exec ./build/guandan_server "$WS_PORT" "$HTTP_PORT" "$DB_PATH"
