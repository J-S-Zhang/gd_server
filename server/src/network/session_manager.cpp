#include "network/session_manager.h"
#include "utils/uuid.h"

/// @file session_manager.cpp
/// @brief 会话创建、玩家绑定与消息下发

namespace guandan {

/// 确保会话条目存在
void SessionManager::ensureSession(uint64_t sessionId) {
    std::lock_guard lock(mutex_);
    if (sessions_.count(sessionId) == 0) {
        Session s;
        s.sessionId = sessionId;
        sessions_[sessionId] = s;
    }
}

/// 创建新会话并返回 ID
uint64_t SessionManager::createSession() {
    std::lock_guard lock(mutex_);
    uint64_t id = nextSessionId_++;
    Session s;
    s.sessionId = id;
    s.token = generateUuid();
    sessions_[id] = s;
    return id;
}

/// 移除会话并解绑玩家
void SessionManager::removeSession(uint64_t sessionId) {
    std::lock_guard lock(mutex_);
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) return;
    if (it->second.playerId > 0) {
        auto psIt = playerSessions_.find(it->second.playerId);
        if (psIt != playerSessions_.end() && psIt->second == sessionId) {
            playerSessions_.erase(psIt);
        }
    }
    sessions_.erase(it);
}

/// 校验并标记会话已认证
bool SessionManager::authenticate(uint64_t sessionId, const std::string& token) {
    std::lock_guard lock(mutex_);
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) return false;
    // MVP: accept any non-empty token or matching session token
    if (token.empty()) return false;
    it->second.authenticated = true;
    it->second.token = token;
    return true;
}

/// 实现 getSession 相关逻辑
Session* SessionManager::getSession(uint64_t sessionId) {
    std::lock_guard lock(mutex_);
    auto it = sessions_.find(sessionId);
    return it != sessions_.end() ? &it->second : nullptr;
}

/// 按玩家 ID 查找会话
Session* SessionManager::findByPlayerId(PlayerId playerId) {
    std::lock_guard lock(mutex_);
    auto sit = playerSessions_.find(playerId);
    if (sit == playerSessions_.end()) return nullptr;
    auto it = sessions_.find(sit->second);
    return it != sessions_.end() ? &it->second : nullptr;
}

/// 查询玩家绑定的会话 ID
uint64_t SessionManager::findSessionIdByPlayerId(PlayerId playerId) {
    std::lock_guard lock(mutex_);
    auto it = playerSessions_.find(playerId);
    return it != playerSessions_.end() ? it->second : 0;
}

/// 将会话绑定到玩家 ID
PlayerId SessionManager::bindPlayer(uint64_t sessionId, PlayerId playerId,
                                    const std::string& nickname) {
    std::lock_guard lock(mutex_);
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) return 0;
    if (playerId == 0) {
        playerId = 10000 + sessionId;
    }
    const uint64_t generation = ++playerBindGeneration_[playerId];
    it->second.playerId = playerId;
    it->second.nickname = nickname;
    it->second.authenticated = true;
    it->second.bindGeneration = generation;
    playerSessions_[playerId] = sessionId;
    return playerId;
}

/// 向指定会话发送消息
void SessionManager::sendTo(uint64_t sessionId, const std::string& message) {
    if (sendCallback_) sendCallback_(sessionId, message);
}

/// 向玩家对应会话发送消息
void SessionManager::sendToPlayer(PlayerId playerId, const std::string& message) {
    auto sid = findSessionIdByPlayerId(playerId);
    if (sid > 0) sendTo(sid, message);
}

}  // namespace guandan
