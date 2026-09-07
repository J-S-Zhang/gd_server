#include "network/session_manager.h"
#include "utils/uuid.h"

namespace guandan {

void SessionManager::ensureSession(uint64_t sessionId) {
    std::lock_guard lock(mutex_);
    if (sessions_.count(sessionId) == 0) {
        Session s;
        s.sessionId = sessionId;
        sessions_[sessionId] = s;
    }
}

uint64_t SessionManager::createSession() {
    std::lock_guard lock(mutex_);
    uint64_t id = nextSessionId_++;
    Session s;
    s.sessionId = id;
    s.token = generateUuid();
    sessions_[id] = s;
    return id;
}

void SessionManager::removeSession(uint64_t sessionId) {
    std::lock_guard lock(mutex_);
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) return;
    if (it->second.playerId > 0) {
        playerSessions_.erase(it->second.playerId);
    }
    sessions_.erase(it);
}

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

Session* SessionManager::getSession(uint64_t sessionId) {
    std::lock_guard lock(mutex_);
    auto it = sessions_.find(sessionId);
    return it != sessions_.end() ? &it->second : nullptr;
}

Session* SessionManager::findByPlayerId(PlayerId playerId) {
    std::lock_guard lock(mutex_);
    auto sit = playerSessions_.find(playerId);
    if (sit == playerSessions_.end()) return nullptr;
    auto it = sessions_.find(sit->second);
    return it != sessions_.end() ? &it->second : nullptr;
}

uint64_t SessionManager::findSessionIdByPlayerId(PlayerId playerId) {
    std::lock_guard lock(mutex_);
    auto it = playerSessions_.find(playerId);
    return it != playerSessions_.end() ? it->second : 0;
}

PlayerId SessionManager::bindPlayer(uint64_t sessionId, PlayerId playerId,
                                    const std::string& nickname) {
    std::lock_guard lock(mutex_);
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) return 0;
    if (playerId == 0) {
        playerId = 10000 + sessionId;
    }
    it->second.playerId = playerId;
    it->second.nickname = nickname;
    it->second.authenticated = true;
    playerSessions_[playerId] = sessionId;
    return playerId;
}

void SessionManager::sendTo(uint64_t sessionId, const std::string& message) {
    if (sendCallback_) sendCallback_(sessionId, message);
}

void SessionManager::sendToPlayer(PlayerId playerId, const std::string& message) {
    auto sid = findSessionIdByPlayerId(playerId);
    if (sid > 0) sendTo(sid, message);
}

}  // namespace guandan
