#pragma once

#include "game/types.h"
#include <string>
#include <vector>

namespace guandan {

/// 客户端 WebSocket 协议消息（与 JSON 字段对应）
struct Message {
    int protocolVersion = 1;              ///< 协议版本
    std::string type;                     ///< 消息类型名
    uint64_t requestId = 0;               ///< 请求 id，用于响应关联
    std::string roomId;                   ///< 房间 id
    uint64_t turnId = 0;                  ///< 回合 id（校验用）
    uint64_t stateVersion = 0;            ///< 状态版本（校验用）
    ErrorCode errorCode = ErrorCode::OK;  ///< 错误或 OK
    std::string dataJson;                 ///< 业务 JSON 载荷
    std::vector<CardId> cards;            ///< 出牌等场景的牌 id 列表
    std::string token;                    ///< 登录 token 等
};

/// 序列化为 JSON 字符串
std::string messageToJson(const Message& msg);
/// 从 JSON 解析 Message
Message parseMessage(const std::string& json);
/// 从 JSON 数组片段解析牌 id 列表
std::vector<CardId> parseCardsArray(const std::string& json);

}  // namespace guandan
