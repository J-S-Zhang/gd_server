# 文档 WebRTC 方案 — SFU

与 `guandan_server` **分离进程**：本服务只做 **WebRTC 媒体转发** + **SFU WebSocket 信令**（SDP/ICE）。

- 端口默认 **4444**（客户端 `ServerConfig.documentSfuWsUrl`，主机与登录公网 IP 一致）。
- 游戏内开麦状态仍走游戏 WSS：`voice_state` / `voice_speaking`。

```bash
cd server/webrtc-sfu
npm install
npm start
```

Docker：`deploy/docker-compose.yml` 服务 `webrtc-sfu`。

旧 Mesh P2P 代码：`client/lib/voice/webrtc/mesh/webrtc_mesh_peer.dart`（对照学习）。
