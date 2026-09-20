#!/bin/bash
# 方案 A（Nginx 反代）：对外 80，本机 8080 + 9001
# 用法：./scripts/start.sh [ws_port] [http_port] [db_path] [app_version_json] [avatars_dir]
set -e
cd "$(dirname "$0")/.."
mkdir -p logs

WS_PORT="${1:-9001}"
HTTP_PORT="${2:-8080}"
DB_PATH="${3:-./server/data/users.json}"
APP_VERSION_PATH="${4:-./server/config/app_version.json}"
AVATARS_DIR="${5:-./downloads/avatars}"

VOICE_PORT="${6:-9002}"

echo "启动 guandan_voice_server UDP=$VOICE_PORT"
./build/guandan_voice_server "$VOICE_PORT" &
VOICE_PID=$!

echo "启动 guandan_server WS=$WS_PORT HTTP=$HTTP_PORT DB=$DB_PATH VERSION=$APP_VERSION_PATH AVATARS=$AVATARS_DIR"
trap 'kill $VOICE_PID 2>/dev/null || true' EXIT
exec ./build/guandan_server "$WS_PORT" "$HTTP_PORT" "$DB_PATH" "$APP_VERSION_PATH" "$AVATARS_DIR"
