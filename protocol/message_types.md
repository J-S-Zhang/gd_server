# 消息类型

## 客户端请求

| type | 说明 | data 字段 |
|------|------|-----------|
| login | 登录（WS 鉴权） | `{ "token": "..." }` |
| create_room | 创建房间 | `{}` |
| join_room | 加入房间 | `{ "room_id": "938271" }` |
| leave_room | 离开房间 | `{}` |
| ready | 玩家准备 | `{}` |
| start_game | 开始游戏（房主） | `{}` |
| play_cards | 出牌 | `{ "cards": [10, 23, 41] }` |
| pass | 过牌 | `{}` |
| ping | 心跳 | `{}` |

## 服务器广播 / 响应

| type | 说明 |
|------|------|
| login_result | 登录结果 |
| room_created | 房间创建成功 |
| room_joined | 加入房间成功 |
| room_state | 房间状态更新 |
| player_joined | 玩家加入 |
| player_left | 玩家离开 |
| player_ready | 玩家准备 |
| game_started | 游戏开始 |
| cards_dealt | 发牌（仅自己的手牌） |
| player_played | 玩家出牌 |
| player_passed | 玩家过牌 |
| turn_changed | 轮到下一玩家 |
| game_over | 游戏结束 |
| settlement | 结算结果 |
| game_snapshot | 断线重连快照 |
| error | 错误响应 |
| pong | 心跳响应 |

## 示例

### 创建房间

请求：
```json
{ "protocol_version": 1, "type": "create_room", "request_id": 1 }
```

响应：
```json
{
  "type": "room_created",
  "request_id": 1,
  "data": { "room_id": "938271" }
}
```

### 出牌

请求：
```json
{
  "protocol_version": 1,
  "type": "play_cards",
  "request_id": 4,
  "room_id": "938271",
  "turn_id": 37,
  "data": { "cards": [10, 23, 41] }
}
```

广播：
```json
{
  "type": "player_played",
  "room_id": "938271",
  "data": {
    "player_id": 10001,
    "cards": [10, 23, 41],
    "pattern": "triple",
    "next_player": 10002,
    "state_version": 29,
    "turn_id": 38
  }
}
```
