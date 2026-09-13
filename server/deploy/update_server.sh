#!/bin/bash
# 在 Linux 游戏服务器上更新并重启 guandan_server（含 seat_chat / voice 等新消息）
set -euo pipefail

ROOT="${1:-/home/gd_server}"
WS_PORT="${2:-9001}"
HTTP_PORT="${3:-8080}"
USERS_JSON="${4:-$ROOT/server/data/users.json}"
BINARY="$ROOT/build/guandan_server"

echo "==> 工作目录: $ROOT"
cd "$ROOT"

if [ -d server/build ]; then
  echo "==> 编译服务端..."
  cd server/build
  cmake ..
  cmake --build . -j"$(nproc)"
  cd "$ROOT"
elif [ -d build ]; then
  echo "==> 编译服务端..."
  cd build
  cmake ..
  cmake --build . -j"$(nproc)"
  cd "$ROOT"
else
  echo "未找到 build 目录，请先: mkdir -p server/build && cd server/build && cmake .."
  exit 1
fi

if [ ! -x "$BINARY" ]; then
  BINARY="$(find "$ROOT" -name guandan_server -type f -perm -111 | head -n1)"
fi
if [ -z "${BINARY:-}" ] || [ ! -x "$BINARY" ]; then
  echo "找不到 guandan_server 可执行文件"
  exit 1
fi

echo "==> 停止旧进程..."
pkill -f guandan_server || true
sleep 1

echo "==> 启动: $BINARY $WS_PORT $HTTP_PORT $USERS_JSON"
nohup "$BINARY" "$WS_PORT" "$HTTP_PORT" "$USERS_JSON" > "$ROOT/logs/server.log" 2>&1 &
sleep 1

if ss -tlnp 2>/dev/null | grep -q ":$WS_PORT"; then
  echo "==> 更新成功，WebSocket 端口 $WS_PORT 已监听"
else
  echo "==> 启动可能失败，请查看 $ROOT/logs/server.log"
  exit 1
fi
