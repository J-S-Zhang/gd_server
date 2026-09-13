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

bool extractJsonBoolFromData(const std::string& json, const std::string& key) {
    const std::string needle = "\"" + key + "\":";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return false;
    pos += needle.size();
    while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) ++pos;
    return json.compare(pos, 4, "true") == 0;
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

void MessageDispatcher::sendPlayerView(const std::shared_ptr<Room>& room,
                                     PlayerId playerId,
                                     const std::string& messageType) {
    const int anchor = room->resolveViewAnchor(playerId);
    auto view = room->engine().buildViewFor(playerId, anchor);
    Message msg;
    msg.type = messageType;
    msg.roomId = room->id();
    msg.dataJson = buildGameSnapshotJson(view, room->id());
    sessionManager_.sendToPlayer(playerId, messageToJson(msg));
}

void MessageDispatcher::sendSnapshotToPlayer(const std::shared_ptr<Room>& room,
                                             PlayerId playerId) {
    sendPlayerView(room, playerId, "game_snapshot");
}

void MessageDispatcher::sendSpectateUpdatesToFinishedPlayers(
    const std::shared_ptr<Room>& room) {
    const auto& state = room->engine().getState();
    if (state.phase != GamePhase::PLAYING) return;

    for (const auto& p : room->players()) {
        int viewerSeat = -1;
        for (int i = 0; i < state.playerCount; ++i) {
            if (state.players[i].id == p.id) {
                viewerSeat = i;
                break;
            }
        }
        if (viewerSeat < 0 || !state.players[viewerSeat].hasFinished) continue;
        sendPlayerView(room, p.id, "spectate_update");
    }
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
        executeBotTurn(room, currentPlayer);
    });
}

void MessageDispatcher::broadcastPlayerPlayed(
    const std::shared_ptr<Room>& room,
    PlayerId playerId,
    const std::vector<CardId>& cards,
    const PlayResult& result
) {
    (void)result;
    const auto& state = room->engine().getState();
    int playedSeat = -1;
    for (int i = 0; i < state.playerCount; ++i) {
        if (state.players[i].id == playerId) {
            playedSeat = i;
            break;
        }
    }
    const auto& playedPlayer = playedSeat >= 0 ? state.players[playedSeat] : state.players[0];

    Message played;
    played.type = "player_played";
    played.roomId = room->id();
    played.dataJson = buildPlayerPlayedJson(
        playerId, cards,
        static_cast<int>(playedPlayer.hand.size()),
        playedPlayer.hasFinished,
        playedPlayer.finishRank,
        state.currentPlayerIndex,
        state.stateVersion, state.turnId);
    broadcastToRoom(room, played);
    sendSpectateUpdatesToFinishedPlayers(room);
}

void MessageDispatcher::broadcastPlayerPassed(
    const std::shared_ptr<Room>& room,
    PlayerId playerId,
    const PlayResult& result
) {
    (void)result;
    const auto& state = room->engine().getState();
    int passedSeat = -1;
    for (int i = 0; i < state.playerCount; ++i) {
        if (state.players[i].id == playerId) {
            passedSeat = i;
            break;
        }
    }
    const bool roundReset = !state.lastPattern.isValid;

    Message passed;
    passed.type = "player_passed";
    passed.roomId = room->id();
    passed.dataJson = buildPlayerPassedJson(
        playerId, passedSeat, state.currentPlayerIndex,
        roundReset, state.stateVersion, state.turnId);
    broadcastToRoom(room, passed);
    sendSpectateUpdatesToFinishedPlayers(room);
}

void MessageDispatcher::executeBotTurn(const std::shared_ptr<Room>& room, PlayerId botId) {
    if (!isBotPlayer(botId)) return;

    auto& engine = room->engine();
    const auto& state = engine.getState();
    if (state.phase != GamePhase::PLAYING) return;

    int seat = -1;
    for (int i = 0; i < state.playerCount; ++i) {
        if (state.players[i].id == botId) {
            seat = i;
            break;
        }
    }
    if (seat < 0 || seat != state.currentPlayerIndex) return;

    cancelTurnTimer(room->id());

    if (auto play = engine.chooseBotPlay(botId)) {
        auto result = engine.playCards(botId, *play);
        if (result.code == ErrorCode::OK) {
            Logger::info("Bot " + std::to_string(botId) + " played " +
                         std::to_string(play->size()) + " card(s)");
            broadcastPlayerPlayed(room, botId, *play, result);
            if (result.gameOver) {
                onGameOver(room, result.settlement);
            } else {
                scheduleTurnTimer(room);
            }
            return;
        }
    }

    Logger::info("Bot " + std::to_string(botId) + " passed");
    auto passResult = engine.pass(botId);
    if (passResult.code == ErrorCode::OK) {
        broadcastPlayerPassed(room, botId, passResult);
        if (passResult.gameOver) {
            onGameOver(room, passResult.settlement);
        } else {
            scheduleTurnTimer(room);
        }
    }
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
        oss << ",\"mode\":\"" << room->config().modeName << "\"";
        oss << ",\"max_players\":" << room->config().maxPlayers;
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
    onRoundStarted(room);
}

void MessageDispatcher::onRoundStarted(const std::shared_ptr<Room>& room) {
    const auto& tribute = room->engine().lastTributeResult();
    if (!tribute.skipped || tribute.antiTribute || !tribute.tributes.empty()) {
        Message tributeMsg;
        tributeMsg.type = "tribute_resolved";
        tributeMsg.roomId = room->id();
        tributeMsg.dataJson = buildTributeResolvedJson(tribute);
        broadcastToRoom(room, tributeMsg);
    }

    for (const auto& p : room->players()) {
        sendPlayerView(room, p.id, "cards_dealt");
    }
    scheduleTurnTimer(room);
}

void MessageDispatcher::onGameOver(const std::shared_ptr<Room>& room,
                                     const SettlementResult& result) {
    cancelTurnTimer(room->id());

    std::ostringstream oss;
    oss << "{\"winning_team\":" << result.winningTeam;
    oss << ",\"level_upgrade\":" << result.levelUpgrade;
    oss << ",\"new_level\":" << result.newLevel;
    oss << ",\"team_levels\":[" << result.teamLevels[0] << ","
        << result.teamLevels[1] << "]";
    oss << ",\"attacking_team\":" << result.attackingTeam;
    oss << ",\"is_pass_a_round\":" << (result.isPassARound ? "true" : "false");
    oss << ",\"pass_a_team\":" << result.passATeam;
    oss << ",\"pass_a_success\":" << (result.passASuccess ? "true" : "false");
    oss << ",\"match_won\":" << (result.matchWon ? "true" : "false");
    oss << ",\"entered_pass_a_phase\":" << (result.enteredPassAPhase ? "true" : "false");
    oss << ",\"pass_a_fail_count\":" << result.passAFailCount;
    oss << ",\"pass_a_fail_reset\":" << (result.passAFailReset ? "true" : "false");
    oss << ",\"in_pass_a_phase\":["
        << (result.inPassAPhase[0] ? "true" : "false") << ","
        << (result.inPassAPhase[1] ? "true" : "false") << "]";
    oss << ",\"pass_a_fail_counts\":[" << result.passAFailCounts[0] << ","
        << result.passAFailCounts[1] << "]";
    oss << ",\"finish_ranks\":[";
    const auto& state = room->engine().getState();
    for (int i = 0; i < state.playerCount; ++i) {
        if (i > 0) oss << ',';
        oss << "{\"player_id\":" << state.players[i].id
            << ",\"seat_index\":" << state.players[i].seatIndex
            << ",\"finish_rank\":" << state.players[i].finishRank
            << ",\"has_finished\":true}";
    }
    oss << "]}";
    Message settlement;
    settlement.type = "settlement";
    settlement.roomId = room->id();
    settlement.dataJson = oss.str();
    broadcastToRoom(room, settlement);

    if (result.matchWon) {
        Message over;
        over.type = "game_over";
        over.roomId = room->id();
        over.dataJson = oss.str();
        broadcastToRoom(room, over);
        room->finishMatch();
        return;
    }

    if (room->startNextRound()) {
        onRoundStarted(room);
    }
}

void MessageDispatcher::handleSetRoomOptions(uint64_t sessionId, const Message& msg, SendFn send) {
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

    const bool enableTribute = extractJsonBoolFromData(msg.dataJson, "enable_tribute");
    if (!room->setEnableTribute(playerId, enableTribute)) {
        sendError(send, msg.requestId, ErrorCode::INVALID_STATE);
        return;
    }

    Message resp;
    resp.type = "room_options_updated";
    resp.requestId = msg.requestId;
    resp.roomId = room->id();
    resp.dataJson = "{\"enable_tribute\":" + std::string(enableTribute ? "true" : "false") + "}";
    sendResponse(send, resp);

    Message stateMsg;
    stateMsg.type = "room_state";
    stateMsg.roomId = room->id();
    stateMsg.dataJson = buildRoomStateJson(*room);
    broadcastToRoom(room, stateMsg);
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
    broadcastPlayerPlayed(room, playerId, msg.cards, result);

    if (result.gameOver) {
        onGameOver(room, result.settlement);
    } else {
        scheduleTurnTimer(room);
    }

    Message resp;
    resp.type = "player_played";
    resp.requestId = msg.requestId;
    resp.roomId = room->id();
    const auto& state = room->engine().getState();
    int playedSeat = -1;
    for (int i = 0; i < state.playerCount; ++i) {
        if (state.players[i].id == playerId) {
            playedSeat = i;
            break;
        }
    }
    const auto& playedPlayer = playedSeat >= 0 ? state.players[playedSeat] : state.players[0];
    resp.dataJson = buildPlayerPlayedJson(
        playerId, msg.cards,
        static_cast<int>(playedPlayer.hand.size()),
        playedPlayer.hasFinished,
        playedPlayer.finishRank,
        state.currentPlayerIndex,
        state.stateVersion, state.turnId);
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
    broadcastPlayerPassed(room, playerId, result);
    scheduleTurnTimer(room);

    Message resp;
    resp.type = "player_passed";
    resp.requestId = msg.requestId;
    resp.roomId = room->id();
    const auto& state = room->engine().getState();
    int passedSeat = -1;
    for (int i = 0; i < state.playerCount; ++i) {
        if (state.players[i].id == playerId) {
            passedSeat = i;
            break;
        }
    }
    resp.dataJson = buildPlayerPassedJson(
        playerId, passedSeat, state.currentPlayerIndex,
        !state.lastPattern.isValid, state.stateVersion, state.turnId);
    sendResponse(send, resp);
}

void MessageDispatcher::handleSpectateTeammate(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    auto room = roomManager_.findRoomByPlayer(playerId);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::NOT_IN_ROOM);
        return;
    }
    if (room->phase() != RoomPhase::PLAYING) {
        sendError(send, msg.requestId, ErrorCode::INVALID_STATE);
        return;
    }

    const int targetSeat = static_cast<int>(extractJsonUintFromData(msg.dataJson, "target_seat_index"));
    if (!room->setSpectateTarget(playerId, targetSeat)) {
        sendError(send, msg.requestId, ErrorCode::INVALID_STATE);
        return;
    }

    Message resp;
    resp.type = "spectate_changed";
    resp.requestId = msg.requestId;
    resp.roomId = room->id();
    const int anchor = room->resolveViewAnchor(playerId);
    auto view = room->engine().buildViewFor(playerId, anchor);
    resp.dataJson = buildGameSnapshotJson(view, room->id());
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

void MessageDispatcher::handleSeatChat(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    auto room = roomManager_.findRoomByPlayer(playerId);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::NOT_IN_ROOM);
        return;
    }

    const std::string content = extractJsonStringFromData(msg.dataJson, "content");
    if (content.empty() || content.size() > 64) {
        sendError(send, msg.requestId, ErrorCode::INVALID_MESSAGE);
        return;
    }

    int seatIndex = -1;
    for (const auto& p : room->players()) {
        if (p.id == playerId) {
            seatIndex = p.seatIndex;
            break;
        }
    }
    if (seatIndex < 0) {
        sendError(send, msg.requestId, ErrorCode::INVALID_STATE);
        return;
    }

    const bool isEmoji = extractJsonBoolFromData(msg.dataJson, "is_emoji");

    Message broadcast;
    broadcast.type = "seat_chat";
    broadcast.roomId = room->id();
    broadcast.dataJson = std::string("{\"player_id\":") + std::to_string(playerId) +
                         ",\"seat_index\":" + std::to_string(seatIndex) +
                         ",\"content\":\"" + jsonEscape(content) + "\"" +
                         ",\"is_emoji\":" + (isEmoji ? "true" : "false") + "}";
    broadcastToRoom(room, broadcast);

    Message resp;
    resp.type = "seat_chat_ack";
    resp.requestId = msg.requestId;
    resp.roomId = room->id();
    sendResponse(send, resp);
}

void MessageDispatcher::handleVoiceSignal(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId fromId = resolvePlayerId(sessionId);
    auto room = roomManager_.findRoomByPlayer(fromId);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::NOT_IN_ROOM);
        return;
    }

    const PlayerId targetId =
        static_cast<PlayerId>(extractJsonUintFromData(msg.dataJson, "target_player_id"));
    if (targetId == 0 || targetId == fromId) {
        sendError(send, msg.requestId, ErrorCode::INVALID_MESSAGE);
        return;
    }

    bool targetInRoom = false;
    for (const auto& p : room->players()) {
        if (p.id == targetId) {
            targetInRoom = true;
            break;
        }
    }
    if (!targetInRoom) {
        sendError(send, msg.requestId, ErrorCode::INVALID_STATE);
        return;
    }

    const std::string signalType = extractJsonStringFromData(msg.dataJson, "signal_type");
    if (signalType.empty()) {
        sendError(send, msg.requestId, ErrorCode::INVALID_MESSAGE);
        return;
    }

    std::ostringstream oss;
    oss << "{\"from_player_id\":" << fromId;
    oss << ",\"signal_type\":\"" << jsonEscape(signalType) << "\"";

    if (signalType == "offer" || signalType == "answer") {
        const std::string sdp = extractJsonStringFromData(msg.dataJson, "sdp");
        const std::string sdpType = extractJsonStringFromData(msg.dataJson, "type");
        if (sdp.empty() || sdpType.empty()) {
            sendError(send, msg.requestId, ErrorCode::INVALID_MESSAGE);
            return;
        }
        oss << ",\"sdp\":\"" << jsonEscape(sdp) << "\"";
        oss << ",\"type\":\"" << jsonEscape(sdpType) << "\"";
    } else if (signalType == "ice") {
        const std::string candidate = extractJsonStringFromData(msg.dataJson, "candidate");
        const std::string sdpMid = extractJsonStringFromData(msg.dataJson, "sdp_mid");
        const uint64_t mline = extractJsonUintFromData(msg.dataJson, "sdp_mline_index");
        if (candidate.empty()) {
            sendError(send, msg.requestId, ErrorCode::INVALID_MESSAGE);
            return;
        }
        oss << ",\"candidate\":\"" << jsonEscape(candidate) << "\"";
        oss << ",\"sdp_mid\":\"" << jsonEscape(sdpMid) << "\"";
        oss << ",\"sdp_mline_index\":" << mline;
    } else {
        sendError(send, msg.requestId, ErrorCode::INVALID_MESSAGE);
        return;
    }
    oss << "}";

    Message forward;
    forward.type = "voice_signal";
    forward.roomId = room->id();
    forward.dataJson = oss.str();
    sessionManager_.sendToPlayer(targetId, messageToJson(forward));

    Message resp;
    resp.type = "voice_signal_ack";
    resp.requestId = msg.requestId;
    resp.roomId = room->id();
    sendResponse(send, resp);
}

void MessageDispatcher::handleVoiceState(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    auto room = roomManager_.findRoomByPlayer(playerId);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::NOT_IN_ROOM);
        return;
    }

    const bool micEnabled = extractJsonBoolFromData(msg.dataJson, "mic_enabled");
    const bool speakerEnabled = extractJsonBoolFromData(msg.dataJson, "speaker_enabled");

    Message broadcast;
    broadcast.type = "voice_state";
    broadcast.roomId = room->id();
    broadcast.dataJson = std::string("{\"player_id\":") + std::to_string(playerId) +
                         ",\"mic_enabled\":" + (micEnabled ? "true" : "false") +
                         ",\"speaker_enabled\":" + (speakerEnabled ? "true" : "false") + "}";
    broadcastToRoom(room, broadcast);

    Message resp;
    resp.type = "voice_state_ack";
    resp.requestId = msg.requestId;
    resp.roomId = room->id();
    sendResponse(send, resp);
}

void MessageDispatcher::handlePing(SendFn send) {
    Message msg;
    msg.type = "pong";
    sendResponse(send, msg);
}

void MessageDispatcher::broadcastDismissVote(const std::shared_ptr<Room>& room,
                                             const std::string& type) {
    Message msg;
    msg.type = type;
    msg.roomId = room->id();
    msg.dataJson = buildDismissVoteJson(room->dismissVote());
    broadcastToRoom(room, msg);
}

void MessageDispatcher::dissolveRoom(const std::shared_ptr<Room>& room) {
    cancelTurnTimer(room->id());
    const RoomId roomId = room->id();

    Message msg;
    msg.type = "room_dismissed";
    msg.roomId = roomId;
    msg.dataJson = "{}";
    broadcastToRoom(room, msg);

    roomManager_.removeRoom(roomId);
    Logger::info("Room dissolved: " + roomId);
}

void MessageDispatcher::handleRequestDismiss(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    auto room = roomManager_.findRoomByPlayer(playerId);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::NOT_IN_ROOM);
        return;
    }
    if (!room->requestDismiss(playerId)) {
        sendError(send, msg.requestId, ErrorCode::INVALID_STATE);
        return;
    }

    broadcastDismissVote(room, "dismiss_vote_started");

    Message resp;
    resp.type = "dismiss_vote_started";
    resp.requestId = msg.requestId;
    resp.roomId = room->id();
    resp.dataJson = buildDismissVoteJson(room->dismissVote());
    sendResponse(send, resp);

    if (room->dismissVote().allAgreed()) {
        dissolveRoom(room);
    }
}

void MessageDispatcher::handleVoteDismiss(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    auto room = roomManager_.findRoomByPlayer(playerId);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::NOT_IN_ROOM);
        return;
    }
    if (!room->dismissVote().active()) {
        sendError(send, msg.requestId, ErrorCode::INVALID_STATE);
        return;
    }

    const bool agree = extractJsonBoolFromData(msg.dataJson, "agree");
    if (!room->voteDismiss(playerId, agree)) {
        sendError(send, msg.requestId, ErrorCode::INVALID_STATE);
        return;
    }

    Message resp;
    resp.type = agree ? "dismiss_vote_updated" : "dismiss_vote_rejected";
    resp.requestId = msg.requestId;
    resp.roomId = room->id();
    resp.dataJson = buildDismissVoteJson(room->dismissVote());
    sendResponse(send, resp);

    if (!agree || room->dismissVote().hasRejection()) {
        const auto voteSnapshot = room->dismissVote();
        room->cancelDismissVote();
        Message rejected;
        rejected.type = "dismiss_vote_rejected";
        rejected.roomId = room->id();
        rejected.dataJson = buildDismissVoteJson(voteSnapshot);
        broadcastToRoom(room, rejected);
        return;
    }

    broadcastDismissVote(room, "dismiss_vote_updated");

    if (room->dismissVote().allAgreed()) {
        dissolveRoom(room);
    }
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
    else if (msg.type == "set_room_options") handleSetRoomOptions(sessionId, msg, send);
    else if (msg.type == "start_game") handleStartGame(sessionId, msg, send);
    else if (msg.type == "play_cards") handlePlayCards(sessionId, msg, send);
    else if (msg.type == "pass") handlePass(sessionId, msg, send);
    else if (msg.type == "spectate_teammate") handleSpectateTeammate(sessionId, msg, send);
    else if (msg.type == "reconnect") handleReconnect(sessionId, msg, send);
    else if (msg.type == "request_dismiss") handleRequestDismiss(sessionId, msg, send);
    else if (msg.type == "vote_dismiss") handleVoteDismiss(sessionId, msg, send);
    else if (msg.type == "voice_state") handleVoiceState(sessionId, msg, send);
    else if (msg.type == "seat_chat") handleSeatChat(sessionId, msg, send);
    else if (msg.type == "voice_signal") handleVoiceSignal(sessionId, msg, send);
    else if (msg.type == "ping") handlePing(send);
    else sendError(send, msg.requestId, ErrorCode::UNKNOWN_TYPE);
}

}  // namespace guandan
