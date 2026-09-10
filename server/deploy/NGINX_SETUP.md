# Nginx 反向代理部署指南

对外使用标准端口，游戏服务仍监听本机 **8080（HTTP）** 与 **9001（WebSocket）**：

| 对外端口 | 协议 | 转发目标 | 用途 |
|----------|------|----------|------|
| **80** | HTTP | `127.0.0.1:8080` | 登录 / 注册 API |
| **443** | WebSocket（TCP 转发） | `127.0.0.1:9001` | 大厅 / 对局长连接 |

移动端 App 默认连接：

```text
http://121.43.35.218/api/login
ws://121.43.35.218:443
```

---

## 1. 启动游戏服务

```bash
cd /path/to/guandan/server
./build/guandan_server 9001 8080
# 参数：WebSocket端口 HTTP端口
```

确认本机可访问：

```bash
curl -X POST http://127.0.0.1:8080/api/login \
  -H "Content-Type: application/json" \
  -d '{"nickname":"test","password":"123456"}'
```

---

## 2. 安装 Nginx（Ubuntu / Debian）

```bash
sudo apt update
sudo apt install -y nginx
```

CentOS / Alibaba Cloud Linux：

```bash
sudo yum install -y nginx
# 或
sudo dnf install -y nginx
```

---

## 3. 部署 HTTP 配置（80 → 8080）

```bash
sudo cp server/deploy/nginx/guandan-http.conf /etc/nginx/conf.d/guandan-http.conf
sudo nginx -t
sudo systemctl enable nginx
sudo systemctl reload nginx
```

验证：

```bash
curl -X POST http://127.0.0.1/api/login \
  -H "Content-Type: application/json" \
  -d '{"nickname":"test","password":"123456"}'
```

---

## 4. 部署 WebSocket 配置（443 → 9001）

Nginx 的 `stream` 块需写在 **`nginx.conf` 顶层**（与 `http { }` 同级）。

编辑 `/etc/nginx/nginx.conf`，在文件末尾、`http { }` 块之外添加：

```nginx
stream {
    include /etc/nginx/conf.d/guandan-ws-stream.conf;
}
```

复制 stream 配置：

```bash
sudo cp server/deploy/nginx/guandan-ws-stream.conf /etc/nginx/conf.d/guandan-ws-stream.conf
sudo nginx -t
sudo systemctl reload nginx
```

> **说明**：此处为 **TCP 四层转发**，客户端使用 `ws://`（非 `wss://`），仅借用 443 端口穿透运营商限制。若后续有域名与证书，可改为 HTTPS + WSS 终止。

---

## 5. 云服务器安全组

阿里云 / 腾讯云控制台 → **安全组 → 入方向** 放行：

| 端口 | 协议 | 来源 | 说明 |
|------|------|------|------|
| 80 | TCP | 0.0.0.0/0 | HTTP 登录 |
| 443 | TCP | 0.0.0.0/0 | WebSocket |
| 8080 | TCP | 127.0.0.1 或内网 | 可选，建议不对公网开放 |
| 9001 | TCP | 127.0.0.1 或内网 | 可选，建议不对公网开放 |

---

## 6. 重新打包移动端 APK

```powershell
cd client
.\scripts\build_apk.ps1 -ServerHost 121.43.35.218
# 默认 API_PORT=80, WS_PORT=443
```

---

## 7. 故障排查

| 现象 | 检查项 |
|------|--------|
| 手机登录 reset | `curl http://<公网IP>/api/login` 是否通；安全组 80 |
| 进大厅失败 | 安全组 443；`ss -tlnp \| grep 9001`；Nginx stream 是否加载 |
| 502 Bad Gateway | 游戏服务 8080 是否在跑 |
| `nginx -t` 报错 stream | 确认 `stream { }` 在 `nginx.conf` 顶层，不在 `http { }` 内 |

查看 Nginx 日志：

```bash
sudo tail -f /var/log/nginx/error.log
sudo tail -f /var/log/nginx/access.log
```

---

## 8. 完整 nginx.conf 结构示例

```nginx
user www-data;
worker_processes auto;
pid /run/nginx.pid;

events {
    worker_connections 1024;
}

http {
    include /etc/nginx/mime.types;
    include /etc/nginx/conf.d/*.conf;
}

stream {
    include /etc/nginx/conf.d/guandan-ws-stream.conf;
}
```

---

## 9. 本地开发（不经 Nginx）

本地调试仍可直接连 8080 / 9001：

```powershell
flutter run --dart-define=SERVER_HOST=192.168.1.100 --dart-define=API_PORT=8080 --dart-define=WS_PORT=9001
```
