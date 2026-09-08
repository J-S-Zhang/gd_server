#pragma once

#include "game/settlement.h"
#include "protocol/message.h"
#include "auth/auth_service.h"
#include "network/session_manager.h"
#include "room/room_manager.h"
#include "timer/timer_manager.h"
#include <functional>
#include <memory>
#include <string>

namespace guandan {

using SendFn = std::function<void(const std::string&)>;

class MessageDispatcher {
public:
    MessageDispatcher(RoomManager& roomManager, SessionManager& sessionManager,
                      TimerManager& timerManager, AuthService& authService);

    void dispatch(uint64_t sessionId, const std::string& rawJson, SendFn send);
    void setupRoomBroadcast(const std::shared_ptr<Room>& room);

private:
    RoomManager& roomManager_;
    SessionManager& sessionManager_;
    TimerManager& timerManager_;
    AuthService& authService_;
    int turnTimeoutSeconds_ = 30;

    PlayerId resolvePlayerId(uint64_t sessionId);
    void broadcastToRoom(const std::shared_ptr<Room>& room, const Message& msg);
    void sendSnapshotToPlayer(const std::shared_ptr<Room>& room, PlayerId playerId);
    void scheduleTurnTimer(const std::shared_ptr<Room>& room);
    void cancelTurnTimer(const RoomId& roomId);

    void handleLogin(uint64_t sessionId, const Message& msg, SendFn send);
    void handleCreateRoom(uint64_t sessionId, const Message& msg, SendFn send);
    void handleJoinRoom(uint64_t sessionId, const Message& msg, SendFn send);
    void handleLeaveRoom(uint64_t sessionId, const Message& msg, SendFn send);
    void handleReady(uint64_t sessionId, const Message& msg, SendFn send);
    void handleStartGame(uint64_t sessionId, const Message& msg, SendFn send);
    void handlePlayCards(uint64_t sessionId, const Message& msg, SendFn send);
    void handlePass(uint64_t sessionId, const Message& msg, SendFn send);
    void handleReconnect(uint64_t sessionId, const Message& msg, SendFn send);
    void handlePing(SendFn send);

    bool validateTurn(const std::shared_ptr<Room>& room, const Message& msg);

    void sendError(SendFn send, uint64_t requestId, ErrorCode code);
    void sendResponse(SendFn send, const Message& msg);
    void onGameStarted(const std::shared_ptr<Room>& room, uint64_t requestId);
    void onGameOver(const std::shared_ptr<Room>& room, const SettlementResult& result);
};

}  // namespace guandan
