# 错误码

| 代码 | 名称 | 说明 |
|------|------|------|
| 0 | OK | 成功 |
| 1001 | INVALID_TOKEN | Token 无效或过期 |
| 1002 | UNAUTHORIZED | 未授权 |
| 2001 | ROOM_NOT_FOUND | 房间不存在 |
| 2002 | ROOM_FULL | 房间已满 |
| 2003 | ALREADY_IN_ROOM | 已在房间中 |
| 2004 | NOT_IN_ROOM | 不在房间中 |
| 2005 | NOT_ROOM_OWNER | 非房主 |
| 3001 | NOT_YOUR_TURN | 未轮到你出牌 |
| 3002 | INVALID_CARDS | 牌不合法或不在手牌中 |
| 3003 | INVALID_PATTERN | 牌型不合法 |
| 3004 | CANNOT_BEAT | 无法压过上家 |
| 3005 | INVALID_STATE | 当前状态不允许此操作 |
| 3006 | REQUEST_EXPIRED | 请求已过期（turn_id/state_version 不匹配） |
| 3007 | DUPLICATE_REQUEST | 重复请求 |
| 3008 | NOT_ALL_READY | 未全部准备 |
| 4001 | INVALID_MESSAGE | 消息格式错误 |
| 4002 | UNKNOWN_TYPE | 未知消息类型 |
| 5000 | INTERNAL_ERROR | 服务器内部错误 |
