# 方案 A：Nginx 统一 80 端口

```text
手机 App
  ├─ http://121.43.35.218/api/login   → Nginx:80 → 游戏服务:8080
  └─ ws://121.43.35.218/ws            → Nginx:80 → 游戏服务:9001
```

---

## 一、启动游戏服务（本机 8080 + 9001）

```bash
cd /home/gd_server

# 不要用 80 端口！80 留给 Nginx
./guandan_server 9001 8080 /home/gd_server/server/data/users.json
```

或使用脚本：

```bash
chmod +x server/scripts/start.sh
./server/scripts/start.sh 9001 8080 /home/gd_server/server/data/users.json
```

确认监听：

```bash
ss -tlnp | grep guandan
# 应看到 :8080 和 :9001（不是 :80）
```

---

## 二、安装配置 Nginx

```bash
cd /home/gd_server
chmod +x server/deploy/install_nginx.sh
sudo server/deploy/install_nginx.sh
```

或手动：

```bash
sudo cp server/deploy/nginx/guandan.conf /etc/nginx/conf.d/guandan.conf
sudo rm -f /etc/nginx/sites-enabled/default
sudo nginx -t
sudo systemctl reload nginx
```

确认 Nginx 占用 80：

```bash
ss -tlnp | grep ':80'
# 应是 nginx
```

---

## 三、阿里云安全组

入方向只需放行 **TCP 80**。

8080、9001 仅本机访问，可不对外放行。

---

## 四、验证

```bash
# 登录 API
curl -X POST http://127.0.0.1/api/login \
  -H "Content-Type: application/json" \
  -d '{"nickname":"zjs","password":"你的密码"}'

# WebSocket 握手
curl -i -N \
  -H "Connection: Upgrade" -H "Upgrade: websocket" \
  -H "Sec-WebSocket-Version: 13" \
  -H "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==" \
  http://127.0.0.1/ws
# 期望：HTTP/1.1 101 Switching Protocols
```

---

## 五、手机 APK

客户端默认：

- HTTP：`http://121.43.35.218/api/login`
- WS：`ws://121.43.35.218/ws`

重新安装最新 `app-release.apk`，卸载旧版后再装。

---

## 常见问题

| 现象 | 处理 |
|------|------|
| 登录 404 nginx | 删除 `sites-enabled/default`，确认 guandan.conf 已加载 |
| 大厅一直连接中 | 检查 `/ws` 是否返回 101；游戏服务 9001 是否在跑 |
| 端口冲突 | 游戏服务勿占 80；先 `systemctl stop nginx` 排查再 reload |
| 改了 guandan_server 为 80 80 | 改回 `9001 8080` |
