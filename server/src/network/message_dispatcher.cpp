#include "network/message_dispatcher.h"
#include "game/room_config.h"
#include "protocol/game_view_builder.h"
#include "protocol/message.h"
#include "utils/logger.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>

namespace guandan {

namespace {

std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

const char* roomPhaseToString(RoomPhase phase) {
    switch (phase) {
        case RoomPhase::CREATED: return "CREATED";
        case RoomPhase::WAITING: return "WAITING";
        case RoomPhase::PLAYING: return "PLAYING";
        case RoomPhase::SETTLEMENT: return "SETTLEMENT";
        case RoomPhase::FINISHED: return "FINISHED";
    }
    return "WAITING";
}

uint64_t extractJsonUintFromData(const std::string& json, const std::string& key) {
    const std::string needle = "\"" + key + "\":";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return 0;
    pos += needle.size();
    size_t end = pos;
    while (end < json.size() && (std::isdigit(static_cast<unsigned char>(json[end])) || json[end] == '-')) {
        ++end;
    }
    if (end == pos) return 0;
    try {
        return std::stoull(json.substr(pos, end - pos));
    } catch (...) {
        return 0;
    }
}

std::string extractJsonStringFromData(const std::string& json, const std::string& key) {
    const std::string needle = "\"" + key + "\":\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos += needle.size();
    auto end = json.find('"', pos);
    if (end == std::string::npos) return "";
    return json.substr(pos, end - pos);
}

}  // namespace

MessageDispatcher::MessageDispatcher(RoomManager& roomManager,
                                     SessionManager& sessionManager,
                                     TimerManager& timerManager,
                                     AuthService& authService)
    : roomManager_(roomManager),
      sessionManager_(sessionManager),
      timerManager_(timerManager),
      authService_(authService) {}

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

    const int botDelaySeconds = isBotPlayer(currentPlayer) ? 1 : turnTimeoutSeconds_;

    timerManager_.schedule("turn:" + room->id(), botDelaySeconds, [this, room, currentPlayer]() {
        Logger::info("Auto pass for player " + std::to_string(currentPlayer));
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
    auto user = authService_.validateToken(msg.token);
    if (!user) {
        sendError(send, msg.requestId, ErrorCode::UNAUTHORIZED);
        return;
    }

    sessionManager_.ensureSession(sessionId);
    PlayerId playerId = sessionManager_.bindPlayer(sessionId, user->id, user->nickname);
    sessionManager_.authenticate(sessionId, msg.token);

    auto room = roomManager_.findRoomByPlayer(playerId);

    std::ostringstream oss;
    oss << "{\"player_id\":" << playerId;
    oss << ",\"nickname\":\"" << jsonEscape(user->nickname) << "\"";
    oss << ",\"success\":true";
    if (room && room->phase() != RoomPhase::FINISHED) {
        oss << ",\"in_room\":true";
        oss << ",\"room_id\":\"" << room->id() << "\"";
        oss << ",\"room_phase\":\"" << roomPhaseToString(room->phase()) << "\"";
        oss << ",\"is_owner\":" << (room->ownerId() == playerId ? "true" : "false");
    } else {
        oss << ",\"in_room\":false";
    }
    oss << "}";

    Message resp;
    resp.type = "login_result";
    resp.requestId = msg.requestId;
    resp.dataJson = oss.str();
    sendResponse(send, resp);

    // 若玩家已在房间中，同步房间/对局状态
    if (room && room->phase() != RoomPhase::FINISHED) {
        if (room->phase() == RoomPhase::PLAYING ||
            room->phase() == RoomPhase::SETTLEMENT) {
            sendSnapshotToPlayer(room, playerId);
        } else {
            Message stateMsg;
            stateMsg.type = "room_state";
            stateMsg.roomId = room->id();
            stateMsg.dataJson = buildRoomStateJson(*room);
            sendResponse(send, stateMsg);
        }
        Logger::info("Player " + std::to_string(playerId) + " rejoined room " + room->id());
    }
}

void MessageDispatcher::handleCreateRoom(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    auto* session = sessionManager_.getSession(sessionId);
    std::string nickname = session ? session->nickname : "Player";
    std::string mode = extractJsonStringFromData(msg.dataJson, "mode");
    if (mode.empty()) mode = "six";

    auto room = roomManager_.createRoom(playerId, nickname, mode);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::ALREADY_IN_ROOM);
        return;
    }
    setupRoomBroadcast(room);

    Message resp;
    resp.type = "room_created";
    resp.requestId = msg.requestId;
    resp.dataJson = "{\"room_id\":\"" + room->id() + "\",\"mode\":\"" + room->config().modeName +
                    "\",\"max_players\":" + std::to_string(room->config().maxPlayers) + "}";
    sendResponse(send, resp);

    Message stateMsg;
    stateMsg.type = "room_state";
    stateMsg.roomId = room->id();
    stateMsg.dataJson = buildRoomStateJson(*room);
    broadcastToRoom(room, stateMsg);

    Logger::info("Room created: " + room->id() + " mode=" + room->config().modeName);
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
    resp.dataJson = buildRoomStateJson(*room);
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
    stateMsg.dataJson = buildRoomStateJson(*room);
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
        stateMsg.dataJson = buildRoomStateJson(*remaining);
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
    stateMsg.dataJson = buildRoomStateJson(*room);
    broadcastToRoom(room, stateMsg);
}

void MessageDispatcher::handleUnready(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    auto room = roomManager_.findRoomByPlayer(playerId);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::NOT_IN_ROOM);
        return;
    }
    if (!room->unready(playerId)) {
        sendError(send, msg.requestId, ErrorCode::INVALID_STATE);
        return;
    }

    Message resp;
    resp.type = "player_unready";
    resp.requestId = msg.requestId;
    resp.roomId = room->id();
    resp.dataJson = "{\"player_id\":" + std::to_string(playerId) + "}";
    broadcastToRoom(room, resp);

    Message stateMsg;
    stateMsg.type = "room_state";
    stateMsg.roomId = room->id();
    stateMsg.dataJson = buildRoomStateJson(*room);
    broadcastToRoom(room, stateMsg);
}

void MessageDispatcher::handleChangeSeat(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    auto room = roomManager_.findRoomByPlayer(playerId);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::NOT_IN_ROOM);
        return;
    }

    int seatIndex = static_cast<int>(extractJsonUintFromData(msg.dataJson, "seat_index"));
    if (!room->changeSeat(playerId, seatIndex)) {
        sendError(send, msg.requestId, ErrorCode::INVALID_STATE);
        return;
    }

    Message resp;
    resp.type = "seat_changed";
    resp.requestId = msg.requestId;
    resp.roomId = room->id();
    resp.dataJson = "{\"player_id\":" + std::to_string(playerId) +
                    ",\"seat_index\":" + std::to_string(seatIndex) + "}";
    broadcastToRoom(room, resp);

    Message stateMsg;
    stateMsg.type = "room_state";
    stateMsg.roomId = room->id();
    stateMsg.dataJson = buildRoomStateJson(*room);
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
        stateMsg.dataJson = buildRoomStateJson(*room);
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
    else if (msg.type == "unready") handleUnready(sessionId, msg, send);
    else if (msg.type == "change_seat") handleChangeSeat(sessionId, msg, send);
    else if (msg.type == "start_game") handleStartGame(sessionId, msg, send);
    else if (msg.type == "play_cards") handlePlayCards(sessionId, msg, send);
    else if (msg.type == "pass") handlePass(sessionId, msg, send);
    else if (msg.type == "reconnect") handleReconnect(sessionId, msg, send);
    else if (msg.type == "ping") handlePing(send);
    else sendError(send, msg.requestId, ErrorCode::UNKNOWN_TYPE);
}

}  // namespace guandan
