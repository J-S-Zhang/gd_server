# 通信协议 V1

协议版本：`1`

## 通用消息格式

### 客户端 → 服务器

```json
{
  "protocol_version": 1,
  "type": "message_type",
  "request_id": 10086,
  "room_id": "938271",
  "turn_id": 37,
  "state_version": 28,
  "data": {}
}
```

### 服务器 → 客户端

```json
{
  "protocol_version": 1,
  "type": "message_type",
  "request_id": 10086,
  "room_id": "938271",
  "error_code": 0,
  "data": {}
}
```

## 字段说明

| 字段 | 类型 | 说明 |
|------|------|------|
| protocol_version | int | 协议版本号 |
| type | string | 消息类型 |
| request_id | int | 客户端请求 ID，用于幂等 |
| room_id | string | 6 位房间号 |
| turn_id | int | 当前回合 ID |
| state_version | int | 游戏状态版本号 |
| error_code | int | 错误码，0 表示成功 |
| data | object | 业务数据 |

## 连接

- HTTP API：`https://api.example.com`
- WebSocket：`wss://game.example.com/ws`
- 连接时需携带 Token：`Authorization: Bearer <token>`

## 心跳

客户端每 15 秒发送：

```json
{ "type": "ping", "request_id": 0 }
```

服务器响应：

```json
{ "type": "pong" }
```
