#pragma once

#include "game/types.h"
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

namespace guandan {

/// WebSocket 会话：认证与玩家绑定
struct Session {
    uint64_t sessionId = 0;       ///< 会话 id
    PlayerId playerId = 0;        ///< 绑定的玩家 id，0 表示未绑定
    std::string token;            ///< 登录 token
    std::string nickname;         ///< 昵称缓存
    bool authenticated = false;   ///< 是否已通过 token 认证
    uint64_t bindGeneration = 0;  ///< 绑定代次，用于踢掉旧连接
};

/// 会话生命周期与按玩家/会话发消息
class SessionManager {
public:
    using SendCallback = std::function<void(uint64_t sessionId, const std::string& message)>;  ///< 底层发送回调

    /// 创建新会话并返回 sessionId
    uint64_t createSession();
    /// 移除会话
    void removeSession(uint64_t sessionId);

    /// 用 token 认证会话
    bool authenticate(uint64_t sessionId, const std::string& token);
    /// 按 sessionId 获取会话，不存在返回 nullptr
    Session* getSession(uint64_t sessionId);
    /// 按玩家 id 查找会话
    Session* findByPlayerId(PlayerId playerId);
    /// 按玩家 id 查找 sessionId，无则返回 0
    uint64_t findSessionIdByPlayerId(PlayerId playerId);

    /// 设置全局发送回调
    void setSendCallback(SendCallback cb) { sendCallback_ = std::move(cb); }
    /// 向指定会话发送 JSON 消息
    void sendTo(uint64_t sessionId, const std::string& message);
    /// 向玩家当前会话发送消息
    void sendToPlayer(PlayerId playerId, const std::string& message);

    /// 绑定玩家到会话，返回 playerId
    PlayerId bindPlayer(uint64_t sessionId, PlayerId playerId, const std::string& nickname);

    /// 确保 sessionId 对应条目存在（重连等场景）
    void ensureSession(uint64_t sessionId);

private:
    std::mutex mutex_;                                      ///< 保护所有映射
    uint64_t nextSessionId_ = 1;                            ///< 下一个分配的 sessionId
    std::unordered_map<uint64_t, Session> sessions_;        ///< 会话表
    std::unordered_map<PlayerId, uint64_t> playerSessions_; ///< 玩家 -> 当前 sessionId
    std::unordered_map<PlayerId, uint64_t> playerBindGeneration_;  ///< 玩家绑定代次
    SendCallback sendCallback_;                             ///< 发送实现
};

}  // namespace guandan
