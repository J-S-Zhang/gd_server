#include "network/message_dispatcher.h"
#include "protocol/game_view_builder.h"
#include "protocol/message.h"
#include "utils/logger.h"
#include <algorithm>
#include <sstream>

namespace guandan {

MessageDispatcher::MessageDispatcher(RoomManager& roomManager,
                                     SessionManager& sessionManager,
                                     TimerManager& timerManager)
    : roomManager_(roomManager),
      sessionManager_(sessionManager),
      timerManager_(timerManager) {}

PlayerId MessageDispatcher::resolvePlayerId(uint64_t sessionId) {
    sessionManager_.ensureSession(sessionId);
    auto* session = sessionManager_.getSession(sessionId);
    if (!session || session->playerId == 0) {
        return sessionManager_.bindPlayer(sessionId, 0, "Player" + std::to_string(sessionId));
    }
    return session->playerId;
}

void MessageDispatcher::sendError(SendFn send, uint64_t requestId, ErrorCode code) {
    Message msg;
    msg.type = "error";
    msg.requestId = requestId;
    msg.errorCode = code;
    send(messageToJson(msg));
}

void MessageDispatcher::sendResponse(SendFn send, const Message& msg) {
    send(messageToJson(msg));
}

void MessageDispatcher::broadcastToRoom(const std::shared_ptr<Room>& room,
                                        const Message& msg) {
    for (const auto& p : room->players()) {
        sessionManager_.sendToPlayer(p.id, messageToJson(msg));
    }
}

void MessageDispatcher::setupRoomBroadcast(const std::shared_ptr<Room>& room) {
    room->setBroadcastCallback([this, room](PlayerId playerId, const std::string& json) {
        sessionManager_.sendToPlayer(playerId, json);
    });
}

void MessageDispatcher::sendSnapshotToPlayer(const std::shared_ptr<Room>& room,
                                             PlayerId playerId) {
    auto view = room->engine().buildViewFor(playerId);
    Message msg;
    msg.type = "game_snapshot";
    msg.roomId = room->id();
    msg.dataJson = buildGameSnapshotJson(view, room->id());
    sessionManager_.sendToPlayer(playerId, messageToJson(msg));
}

void MessageDispatcher::cancelTurnTimer(const RoomId& roomId) {
    timerManager_.cancel("turn:" + roomId);
}

void MessageDispatcher::scheduleTurnTimer(const std::shared_ptr<Room>& room) {
    cancelTurnTimer(room->id());
    const auto& state = room->engine().getState();
    if (state.phase != GamePhase::PLAYING) return;

    int currentIdx = state.currentPlayerIndex;
    if (currentIdx < 0 || currentIdx >= state.playerCount) return;
    PlayerId currentPlayer = state.players[currentIdx].id;

    timerManager_.schedule("turn:" + room->id(), turnTimeoutSeconds_, [this, room, currentPlayer]() {
        Logger::info("Turn timeout, auto pass for player " + std::to_string(currentPlayer));
        auto result = room->engine().pass(currentPlayer);
        if (result.code == ErrorCode::OK) {
            const auto& st = room->engine().getState();
            Message msg;
            msg.type = "player_passed";
            msg.roomId = room->id();
            msg.dataJson = buildPlayerPassedJson(
                st.currentPlayerIndex, st.stateVersion, st.turnId);
            broadcastToRoom(room, msg);
            scheduleTurnTimer(room);
        }
    });
}

bool MessageDispatcher::validateTurn(const std::shared_ptr<Room>& room, const Message& msg) {
    const auto& state = room->engine().getState();
    if (msg.turnId > 0 && msg.turnId != state.turnId) {
        return false;
    }
    return true;
}

void MessageDispatcher::handleLogin(uint64_t sessionId, const Message& msg, SendFn send) {
    if (!sessionManager_.authenticate(sessionId, msg.token)) {
        // MVP: auto-authenticate with any token
        sessionManager_.authenticate(sessionId, msg.token.empty() ? "guest" : msg.token);
    }
    PlayerId playerId = resolvePlayerId(sessionId);

    Message resp;
    resp.type = "login_result";
    resp.requestId = msg.requestId;
    resp.dataJson = "{\"player_id\":" + std::to_string(playerId) + ",\"success\":true}";
    sendResponse(send, resp);

    // 若玩家已在房间中，发送 snapshot
    auto room = roomManager_.findRoomByPlayer(playerId);
    if (room && room->phase() == RoomPhase::PLAYING) {
        sendSnapshotToPlayer(room, playerId);
    }
}

void MessageDispatcher::handleCreateRoom(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    auto* session = sessionManager_.getSession(sessionId);
    std::string nickname = session ? session->nickname : "Player";

    auto room = roomManager_.createRoom(playerId, nickname);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::ALREADY_IN_ROOM);
        return;
    }
    setupRoomBroadcast(room);

    Message resp;
    resp.type = "room_created";
    resp.requestId = msg.requestId;
    resp.dataJson = "{\"room_id\":\"" + room->id() + "\"}";
    sendResponse(send, resp);

    Message stateMsg;
    stateMsg.type = "room_state";
    stateMsg.roomId = room->id();
    stateMsg.dataJson = buildRoomStateJson(room->players());
    broadcastToRoom(room, stateMsg);

    Logger::info("Room created: " + room->id());
}

void MessageDispatcher::handleJoinRoom(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    auto* session = sessionManager_.getSession(sessionId);
    std::string nickname = session ? session->nickname : "Player";

    auto room = roomManager_.joinRoom(msg.roomId, playerId, nickname);
    if (!room) {
        auto existing = roomManager_.getRoom(msg.roomId);
        if (!existing) {
            sendError(send, msg.requestId, ErrorCode::ROOM_NOT_FOUND);
        } else if (existing->isFull()) {
            sendError(send, msg.requestId, ErrorCode::ROOM_FULL);
        } else {
            sendError(send, msg.requestId, ErrorCode::ALREADY_IN_ROOM);
        }
        return;
    }
    setupRoomBroadcast(room);

    Message resp;
    resp.type = "room_joined";
    resp.requestId = msg.requestId;
    resp.roomId = msg.roomId;
    resp.dataJson = buildRoomStateJson(room->players());
    sendResponse(send, resp);

    Message joinBroadcast;
    joinBroadcast.type = "player_joined";
    joinBroadcast.roomId = room->id();
    joinBroadcast.dataJson = "{\"player_id\":" + std::to_string(playerId) +
                             ",\"nickname\":\"" + nickname + "\"}";
    broadcastToRoom(room, joinBroadcast);

    Message stateMsg;
    stateMsg.type = "room_state";
    stateMsg.roomId = room->id();
    stateMsg.dataJson = buildRoomStateJson(room->players());
    broadcastToRoom(room, stateMsg);
}

void MessageDispatcher::handleLeaveRoom(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    auto room = roomManager_.findRoomByPlayer(playerId);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::NOT_IN_ROOM);
        return;
    }
    RoomId roomId = room->id();
    room->leave(playerId);
    roomManager_.removePlayerFromRoom(playerId);

    Message resp;
    resp.type = "player_left";
    resp.requestId = msg.requestId;
    resp.dataJson = "{\"player_id\":" + std::to_string(playerId) + "}";
    sendResponse(send, resp);

    auto remaining = roomManager_.getRoom(roomId);
    if (remaining && !remaining->isEmpty()) {
        Message stateMsg;
        stateMsg.type = "room_state";
        stateMsg.roomId = roomId;
        stateMsg.dataJson = buildRoomStateJson(remaining->players());
        broadcastToRoom(remaining, stateMsg);
    }
}

void MessageDispatcher::handleReady(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    auto room = roomManager_.findRoomByPlayer(playerId);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::NOT_IN_ROOM);
        return;
    }
    room->ready(playerId);

    Message resp;
    resp.type = "player_ready";
    resp.requestId = msg.requestId;
    resp.roomId = room->id();
    resp.dataJson = "{\"player_id\":" + std::to_string(playerId) + "}";
    broadcastToRoom(room, resp);

    Message stateMsg;
    stateMsg.type = "room_state";
    stateMsg.roomId = room->id();
    stateMsg.dataJson = buildRoomStateJson(room->players());
    broadcastToRoom(room, stateMsg);
}

void MessageDispatcher::onGameStarted(const std::shared_ptr<Room>& room, uint64_t requestId) {
    Message started;
    started.type = "game_started";
    started.requestId = requestId;
    started.roomId = room->id();
    broadcastToRoom(room, started);

    for (const auto& p : room->players()) {
        auto view = room->engine().buildViewFor(p.id);
        Message dealt;
        dealt.type = "cards_dealt";
        dealt.roomId = room->id();
        dealt.dataJson = buildGameSnapshotJson(view, room->id());
        sessionManager_.sendToPlayer(p.id, messageToJson(dealt));
    }
    scheduleTurnTimer(room);
}

void MessageDispatcher::onGameOver(const std::shared_ptr<Room>& room,
                                     const SettlementResult& result) {
    cancelTurnTimer(room->id());

    std::ostringstream oss;
    oss << "{\"winning_team\":" << result.winningTeam;
    oss << ",\"level_upgrade\":" << result.levelUpgrade << "}";
    Message over;
    over.type = "game_over";
    over.roomId = room->id();
    over.dataJson = oss.str();
    broadcastToRoom(room, over);

    Message settlement;
    settlement.type = "settlement";
    settlement.roomId = room->id();
    settlement.dataJson = oss.str();
    broadcastToRoom(room, settlement);
}

void MessageDispatcher::handleStartGame(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    auto room = roomManager_.findRoomByPlayer(playerId);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::NOT_IN_ROOM);
        return;
    }
    if (room->ownerId() != playerId) {
        sendError(send, msg.requestId, ErrorCode::NOT_ROOM_OWNER);
        return;
    }
    if (!room->startGame()) {
        sendError(send, msg.requestId, ErrorCode::NOT_ALL_READY);
        return;
    }
    onGameStarted(room, msg.requestId);
    Message resp;
    resp.type = "game_started";
    resp.requestId = msg.requestId;
    resp.roomId = room->id();
    sendResponse(send, resp);
}

void MessageDispatcher::handlePlayCards(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    auto room = roomManager_.findRoomByPlayer(playerId);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::NOT_IN_ROOM);
        return;
    }
    if (!validateTurn(room, msg)) {
        sendError(send, msg.requestId, ErrorCode::REQUEST_EXPIRED);
        return;
    }
    if (msg.cards.empty()) {
        sendError(send, msg.requestId, ErrorCode::INVALID_CARDS);
        return;
    }

    auto result = room->engine().playCards(playerId, msg.cards);
    if (result.code != ErrorCode::OK) {
        sendError(send, msg.requestId, result.code);
        return;
    }

    cancelTurnTimer(room->id());
    const auto& state = room->engine().getState();

    Message played;
    played.type = "player_played";
    played.roomId = room->id();
    played.dataJson = buildPlayerPlayedJson(
        playerId, msg.cards, state.currentPlayerIndex,
        state.stateVersion, state.turnId);
    broadcastToRoom(room, played);

    if (result.gameOver) {
        onGameOver(room, result.settlement);
    } else {
        scheduleTurnTimer(room);
    }

    Message resp;
    resp.type = "player_played";
    resp.requestId = msg.requestId;
    resp.roomId = room->id();
    resp.dataJson = played.dataJson;
    sendResponse(send, resp);
}

void MessageDispatcher::handlePass(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    auto room = roomManager_.findRoomByPlayer(playerId);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::NOT_IN_ROOM);
        return;
    }
    if (!validateTurn(room, msg)) {
        sendError(send, msg.requestId, ErrorCode::REQUEST_EXPIRED);
        return;
    }

    auto result = room->engine().pass(playerId);
    if (result.code != ErrorCode::OK) {
        sendError(send, msg.requestId, result.code);
        return;
    }

    cancelTurnTimer(room->id());
    const auto& state = room->engine().getState();

    Message passed;
    passed.type = "player_passed";
    passed.roomId = room->id();
    passed.dataJson = buildPlayerPassedJson(
        state.currentPlayerIndex, state.stateVersion, state.turnId);
    broadcastToRoom(room, passed);
    scheduleTurnTimer(room);

    Message resp;
    resp.type = "player_passed";
    resp.requestId = msg.requestId;
    resp.roomId = room->id();
    resp.dataJson = passed.dataJson;
    sendResponse(send, resp);
}

void MessageDispatcher::handleReconnect(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    auto room = roomManager_.findRoomByPlayer(playerId);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::NOT_IN_ROOM);
        return;
    }

    Message resp;
    resp.type = "reconnect_ok";
    resp.requestId = msg.requestId;
    resp.roomId = room->id();
    sendResponse(send, resp);

    if (room->phase() == RoomPhase::PLAYING) {
        sendSnapshotToPlayer(room, playerId);
    } else {
        Message stateMsg;
        stateMsg.type = "room_state";
        stateMsg.roomId = room->id();
        stateMsg.dataJson = buildRoomStateJson(room->players());
        sendResponse(send, stateMsg);
    }
}

void MessageDispatcher::handlePing(SendFn send) {
    Message msg;
    msg.type = "pong";
    sendResponse(send, msg);
}

void MessageDispatcher::dispatch(uint64_t sessionId, const std::string& rawJson, SendFn send) {
    Message msg;
    try {
        msg = parseMessage(rawJson);
    } catch (...) {
        sendError(send, 0, ErrorCode::INVALID_MESSAGE);
        return;
    }

    if (msg.type == "login") handleLogin(sessionId, msg, send);
    else if (msg.type == "create_room") handleCreateRoom(sessionId, msg, send);
    else if (msg.type == "join_room") handleJoinRoom(sessionId, msg, send);
    else if (msg.type == "leave_room") handleLeaveRoom(sessionId, msg, send);
    else if (msg.type == "ready") handleReady(sessionId, msg, send);
    else if (msg.type == "start_game") handleStartGame(sessionId, msg, send);
    else if (msg.type == "play_cards") handlePlayCards(sessionId, msg, send);
    else if (msg.type == "pass") handlePass(sessionId, msg, send);
    else if (msg.type == "reconnect") handleReconnect(sessionId, msg, send);
    else if (msg.type == "ping") handlePing(send);
    else sendError(send, msg.requestId, ErrorCode::UNKNOWN_TYPE);
}

}  // namespace guandan
