# Nginx 部署指南（推荐：手机 4G 可用）

手机 4G 常封 **9001** 等非标准端口，但 **80 端口** 通常可用。  
推荐：**对外只开 80**，HTTP 与 WebSocket 都走 80。

| 对外 | 路径 | 转发到 | 用途 |
|------|------|--------|------|
| **80** | `/api/login` 等 | `127.0.0.1:8080` | 登录 / 注册 |
| **80** | `/ws` | `127.0.0.1:9001` | WebSocket 升级 |

客户端连接：

```text
http://121.43.35.218/api/login
ws://121.43.35.218/ws
```

---

## 1. 启动游戏服务（本机 8080 / 9001）

```bash
cd /home/gd_server
./guandan_server 9001 8080 /home/gd_server/server/data/users.json
```

确认：

```bash
ss -tlnp | grep guandan
# :8080 和 :9001
```

---

## 2. 安装并配置 Nginx

```bash
sudo apt install -y nginx   # Ubuntu/Debian

sudo cp server/deploy/nginx/guandan.conf /etc/nginx/conf.d/guandan.conf
sudo rm -f /etc/nginx/sites-enabled/default
sudo rm -f /etc/nginx/conf.d/guandan-http.conf
sudo rm -f /etc/nginx/conf.d/guandan-ws-stream.conf

sudo nginx -t
sudo systemctl enable nginx
sudo systemctl reload nginx
```

确认 Nginx 占用 80：

```bash
ss -tlnp | grep ':80'
```

---

## 3. 安全组

入方向放行 **TCP 80** 即可（8080/9001 仅本机，可不对外）。

---

## 4. 验证

```bash
# HTTP 登录
curl -X POST http://127.0.0.1/api/login \
  -H "Content-Type: application/json" \
  -d '{"nickname":"test","password":"123456"}'

# WebSocket 握手
curl -i -N \
  -H "Connection: Upgrade" -H "Upgrade: websocket" \
  -H "Sec-WebSocket-Version: 13" \
  -H "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==" \
  http://127.0.0.1/ws
# 应返回 101 Switching Protocols
```

---

## 5. 打包 APK

```powershell
cd client
.\scripts\build_apk.ps1 -ServerHost 121.43.35.218
# 默认 API 80 + WS ws://host/ws
```

---

## 本地开发（不经 Nginx）

```powershell
flutter run --dart-define=API_PORT=8080 --dart-define=WS_PORT=9001 --dart-define=WS_PATH=
```

服务端：`./guandan_server 9001 8080`

---

## 不要用 ws://host:443

443 端口期望 **TLS**，明文 WebSocket 会被运营商 reset。

长期生产环境建议：**域名 + HTTPS/WSS（443）**。
