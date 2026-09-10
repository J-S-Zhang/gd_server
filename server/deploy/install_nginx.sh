#!/bin/bash
# 方案 A：Nginx 对外 80 → 内网 8080(HTTP) + 9001(WebSocket)
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

if ! command -v nginx >/dev/null 2>&1; then
  echo "正在安装 Nginx..."
  if command -v apt >/dev/null 2>&1; then
    sudo apt update && sudo apt install -y nginx
  elif command -v yum >/dev/null 2>&1; then
    sudo yum install -y nginx
  else
    echo "请手动安装 Nginx 后重试"; exit 1
  fi
fi

sudo cp "$SCRIPT_DIR/nginx/guandan.conf" /etc/nginx/conf.d/guandan.conf
sudo rm -f /etc/nginx/sites-enabled/default
sudo rm -f /etc/nginx/conf.d/guandan-http.conf
sudo rm -f /etc/nginx/conf.d/guandan-ws-stream.conf

# WebSocket Upgrade 映射（写入 http 块，若已存在则跳过）
if ! grep -q 'guandan_ws_upgrade' /etc/nginx/nginx.conf 2>/dev/null; then
  sudo sed -i '/^http\s*{/a \    # guandan_ws_upgrade\n    map $http_upgrade $connection_upgrade {\n        default upgrade;\n        '"''"' close;\n    }' /etc/nginx/nginx.conf
fi

sudo nginx -t
sudo systemctl enable nginx
sudo systemctl reload nginx

echo "Nginx 已配置：80 → HTTP 8080, /ws → WS 9001"
echo "请启动游戏服务：./guandan_server 9001 8080 /path/to/users.json"
