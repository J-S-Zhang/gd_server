#pragma once

#include "game/types.h"
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

namespace guandan {

struct Session {
    uint64_t sessionId = 0;
    PlayerId playerId = 0;
    std::string token;
    std::string nickname;
    bool authenticated = false;
};

class SessionManager {
public:
    using SendCallback = std::function<void(uint64_t sessionId, const std::string& message)>;

    uint64_t createSession();
    void removeSession(uint64_t sessionId);

    bool authenticate(uint64_t sessionId, const std::string& token);
    Session* getSession(uint64_t sessionId);
    Session* findByPlayerId(PlayerId playerId);
    uint64_t findSessionIdByPlayerId(PlayerId playerId);

    void setSendCallback(SendCallback cb) { sendCallback_ = std::move(cb); }
    void sendTo(uint64_t sessionId, const std::string& message);
    void sendToPlayer(PlayerId playerId, const std::string& message);

    PlayerId bindPlayer(uint64_t sessionId, PlayerId playerId, const std::string& nickname);

    void ensureSession(uint64_t sessionId);

private:
    std::mutex mutex_;
    uint64_t nextSessionId_ = 1;
    std::unordered_map<uint64_t, Session> sessions_;
    std::unordered_map<PlayerId, uint64_t> playerSessions_;
    SendCallback sendCallback_;
};

}  // namespace guandan
