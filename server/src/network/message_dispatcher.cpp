#include "network/message_dispatcher.h"
#include "livekit/livekit_token.h"
#include "game/room_config.h"
#include "protocol/game_view_builder.h"
#include "protocol/message.h"
#include "utils/logger.h"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <sstream>
#include <string>

/// @file message_dispatcher.cpp
/// @brief WebSocket 消息路由与房间/对局业务处理

namespace guandan {

namespace {

/// 转义 JSON 字符串特殊字符
std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    // 逐字符转义 JSON 特殊符号
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

/// 从用户记录构建加入房间信息
PlayerJoinInfo joinInfoFromUser(const UserRecord& user) {
    // 拷贝用户资料到加入房间结构
    PlayerJoinInfo info;
    info.nickname = user.nickname;
    info.avatar = user.avatar;
    info.avatarPreset = user.avatarPreset;
    return info;
}

/// 从会话解析玩家加入信息
PlayerJoinInfo joinInfoFromSession(AuthService& authService, SessionManager& sessionManager,
                                   uint64_t sessionId) {
    PlayerJoinInfo info;
    auto* session = sessionManager.getSession(sessionId);
    // 无会话时使用默认昵称
    if (!session) {
        info.nickname = "Player";
        return info;
    }
    // 有 Token 则优先从用户库取资料
    if (!session->token.empty()) {
        auto user = authService.validateToken(session->token);
        if (user) {
            return joinInfoFromUser(*user);
        }
    }
    // 否则用会话内昵称
    info.nickname = session->nickname.empty() ? "Player" : session->nickname;
    return info;
}

/// 房间阶段枚举转字符串
const char* roomPhaseToString(RoomPhase phase) {
    // 与客户端约定的房间阶段字符串
    switch (phase) {
        case RoomPhase::CREATED: return "CREATED";
        case RoomPhase::WAITING: return "WAITING";
        case RoomPhase::PLAYING: return "PLAYING";
        case RoomPhase::SETTLEMENT: return "SETTLEMENT";
        case RoomPhase::FINISHED: return "FINISHED";
    }
    return "WAITING";
}

/// 从 JSON 片段解析无符号整数
uint64_t extractJsonUintFromData(const std::string& json, const std::string& key) {
    const std::string needle = "\"" + key + "\":";
    auto pos = json.find(needle);
    // 未找到键则视为 0
    if (pos == std::string::npos) return 0;
    pos += needle.size();
    // 扫描数字段（含可选负号）
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

/// 从 JSON 片段解析字符串字段
std::string extractJsonStringFromData(const std::string& json, const std::string& key) {
    const std::string needle = "\"" + key + "\":\"";
    auto pos = json.find(needle);
    // 非字符串字段或缺失
    if (pos == std::string::npos) return "";
    pos += needle.size();
    // 取到下一个未转义的双引号为止
    auto end = json.find('"', pos);
    if (end == std::string::npos) return "";
    return json.substr(pos, end - pos);
}

/// 从 JSON 片段解析布尔字段
bool extractJsonBoolFromData(const std::string& json, const std::string& key) {
    const std::string needle = "\"" + key + "\":";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return false;
    pos += needle.size();
    // 跳过空白后匹配 literal true
    while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) ++pos;
    return json.compare(pos, 4, "true") == 0;
}

}  // namespace

/// 构造并注入房间、会话、定时器与认证依赖
MessageDispatcher::MessageDispatcher(RoomManager& roomManager,
                                     SessionManager& sessionManager,
                                     TimerManager& timerManager,
                                     AuthService& authService)
    : roomManager_(roomManager),
      sessionManager_(sessionManager),
      timerManager_(timerManager),
      authService_(authService) {}  // 依赖由成员初始化列表注入

/// 更新 LiveKit 语音服务配置
void MessageDispatcher::setLiveKitConfig(const LiveKitConfig& config) {
    // 保存全局 LiveKit 连接参数
    liveKit_ = config;
}

/// 解析会话对应的玩家 ID，未绑定时自动绑定
PlayerId MessageDispatcher::resolvePlayerId(uint64_t sessionId) {
    // 保证 session 存在后再查 playerId
    sessionManager_.ensureSession(sessionId);
    auto* session = sessionManager_.getSession(sessionId);
    // 尚未绑定 playerId 则创建默认玩家
    if (!session || session->playerId == 0) {
        return sessionManager_.bindPlayer(sessionId, 0, "Player" + std::to_string(sessionId));
    }
    return session->playerId;
}

/// 向客户端发送标准 error 消息
void MessageDispatcher::sendError(SendFn send, uint64_t requestId, ErrorCode code) {
    // 标准 error 包：type + requestId + errorCode
    Message msg;
    msg.type = "error";
    msg.requestId = requestId;
    msg.errorCode = code;
    send(messageToJson(msg));
}

/// 将已构造 Message 序列化并发送
void MessageDispatcher::sendResponse(SendFn send, const Message& msg) {
    // 经 JSON 序列化写回 WebSocket
    send(messageToJson(msg));
}

/// 向房间内每位玩家推送同一条消息
void MessageDispatcher::broadcastToRoom(const std::shared_ptr<Room>& room,
                                        const Message& msg) {
    // 房间内每位在线玩家各收一份
    for (const auto& p : room->players()) {
        sessionManager_.sendToPlayer(p.id, messageToJson(msg));
    }
}

/// 注册房间级单播回调（引擎推送用）
void MessageDispatcher::setupRoomBroadcast(const std::shared_ptr<Room>& room) {
    // 引擎侧单播 JSON 时走 SessionManager
    room->setBroadcastCallback([this, room](PlayerId playerId, const std::string& json) {
        sessionManager_.sendToPlayer(playerId, json);
    });
}

/// 按观战锚点构建视图并单播给指定玩家
void MessageDispatcher::sendPlayerView(const std::shared_ptr<Room>& room,
                                     PlayerId playerId,
                                     const std::string& messageType) {
    const int anchor = room->resolveViewAnchor(playerId);
    auto view = room->engine().buildViewFor(playerId, anchor);
    // 按玩家视角打包快照 JSON
    Message msg;
    msg.type = messageType;
    msg.roomId = room->id();
    msg.dataJson = buildGameSnapshotJson(view, *room);
    // 单播给目标玩家
    sessionManager_.sendToPlayer(playerId, messageToJson(msg));
}

/// 下发 game_snapshot 类型视图
void MessageDispatcher::sendSnapshotToPlayer(const std::shared_ptr<Room>& room,
                                             PlayerId playerId) {
    // 统一走 game_snapshot 类型
    sendPlayerView(room, playerId, "game_snapshot");
}

/// 已向局内出完牌的玩家推送 spectate_update
void MessageDispatcher::sendSpectateUpdatesToFinishedPlayers(
    const std::shared_ptr<Room>& room) {
    const auto& state = room->engine().getState();
    // 非对局阶段无需观战推送
    if (state.phase != GamePhase::PLAYING) return;

    for (const auto& p : room->players()) {
        int viewerSeat = -1;
        // 定位该玩家在引擎状态中的座位
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

/// 取消该房间的出牌回合定时器
void MessageDispatcher::cancelTurnTimer(const RoomId& roomId) {
    // 键名 turn:<roomId>
    timerManager_.cancel("turn:" + roomId);
}

/// 取消该房间的贡牌阶段定时器
void MessageDispatcher::cancelTributeTimer(const RoomId& roomId) {
    // 键名 tribute:<roomId>
    timerManager_.cancel("tribute:" + roomId);
}

/// 向全员广播当前贡牌阶段视图
void MessageDispatcher::broadcastTributePhaseUpdate(const std::shared_ptr<Room>& room) {
    // 每人一份贡牌阶段视角
    for (const auto& p : room->players()) {
        sendPlayerView(room, p.id, "tribute_phase_updated");
    }
}

/// 贡牌阶段结束：广播结果、发牌视图并启动出牌计时
void MessageDispatcher::onTributePhaseCompleted(const std::shared_ptr<Room>& room) {
    const auto& tribute = room->engine().lastTributeResult();
    // 广播贡牌结算结果
    Message tributeMsg;
    tributeMsg.type = "tribute_resolved";
    tributeMsg.roomId = room->id();
    tributeMsg.dataJson = buildTributeResolvedJson(tribute);
    broadcastToRoom(room, tributeMsg);

    // 贡牌结束后再发一手牌面视图
    for (const auto& p : room->players()) {
        sendPlayerView(room, p.id, "cards_dealt");
    }
    scheduleTurnTimer(room);
}

/// 若仍有待贡动作则调度 1 秒后机器人贡牌
void MessageDispatcher::scheduleTributeTimer(const std::shared_ptr<Room>& room) {
    cancelTributeTimer(room->id());
    if (!room->engine().hasPendingTributeAction()) return;

    // 1 秒后于房间队列执行机器人贡牌
    timerManager_.schedule("tribute:" + room->id(), 1, [this, room]() {
        room->taskQueue().post([this, room]() { executeBotTributeActions(room); });
    });
}

/// 在房间队列上代机器人完成进贡/还贡
void MessageDispatcher::executeBotTributeActions(const std::shared_ptr<Room>& room) {
    auto& engine = room->engine();
    // 无待贡动作则直接返回
    if (!engine.hasPendingTributeAction()) return;

    cancelTributeTimer(room->id());
    const auto& state = engine.getState();
    bool acted = false;

    // 进贡阶段：待贡座位上的机器人自动选牌
    if (state.phase == GamePhase::TRIBUTE) {
        for (int seat : engine.pendingTributerSeats()) {
            if (seat < 0 || seat >= state.playerCount) continue;
            const PlayerId botId = state.players[seat].id;
            if (!isBotPlayer(botId)) continue;
            const auto cardId = engine.chooseBotTributeCard(botId);
            if (!cardId) continue;
            if (engine.submitTribute(botId, *cardId).code == ErrorCode::OK) {
                acted = true;
            }
        }
    } else if (state.phase == GamePhase::RETURN_TRIBUTE) {
        // 还贡阶段：待还贡座位上的机器人自动选牌
        for (int seat : engine.pendingReturnSeats()) {
            if (seat < 0 || seat >= state.playerCount) continue;
            const PlayerId botId = state.players[seat].id;
            if (!isBotPlayer(botId)) continue;
            const auto cardId = engine.chooseBotReturnCard(botId);
            if (!cardId) continue;
            if (engine.submitReturn(botId, *cardId).code == ErrorCode::OK) {
                acted = true;
            }
        }
    }

    if (acted) {
        broadcastTributePhaseUpdate(room);
    }

    // 仍有待贡则继续调度机器人
    if (engine.hasPendingTributeAction()) {
        scheduleTributeTimer(room);
        return;
    }

    // 已全部完成则进入出牌阶段
    if (engine.getState().phase == GamePhase::PLAYING) {
        onTributePhaseCompleted(room);
    }
}

/// 为当前出牌玩家调度回合：Bot 立即行动，真人按 turnTimeoutSeconds_ 超时
void MessageDispatcher::scheduleTurnTimer(const std::shared_ptr<Room>& room) {
    cancelTurnTimer(room->id());
    const auto& state = room->engine().getState();
    if (state.phase != GamePhase::PLAYING) return;

    int currentIdx = state.currentPlayerIndex;
    if (currentIdx < 0 || currentIdx >= state.playerCount) return;
    PlayerId currentPlayer = state.players[currentIdx].id;

    // Bot 立即出牌（0 延迟）；真人仍走 turnTimeoutSeconds_
    if (isBotPlayer(currentPlayer)) {
        room->taskQueue().post([this, room, currentPlayer]() {
            executeBotTurn(room, currentPlayer);
        });
        return;
    }

    // 超时后在房间队列执行托管/超时逻辑
    timerManager_.schedule("turn:" + room->id(), turnTimeoutSeconds_, [this, room, currentPlayer]() {
        room->taskQueue().post([this, room, currentPlayer]() {
            executeBotTurn(room, currentPlayer);
        });
    });
}

/// 广播某玩家出牌事件并更新观战者
void MessageDispatcher::broadcastPlayerPlayed(
    const std::shared_ptr<Room>& room,
    PlayerId playerId,
    const std::vector<CardId>& cards,
    const PlayResult& result
) {
    (void)result;
    const auto& state = room->engine().getState();
    // 查出牌者座位以附带手牌数与名次
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
    // 全员广播并刷新已出完牌观战者
    broadcastToRoom(room, played);
    sendSpectateUpdatesToFinishedPlayers(room);
}

/// 广播某玩家过牌事件并更新观战者
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

    // 构造 player_passed 并广播
    Message passed;
    passed.type = "player_passed";
    passed.roomId = room->id();
    passed.dataJson = buildPlayerPassedJson(
        playerId, passedSeat, state.currentPlayerIndex,
        roundReset, state.stateVersion, state.turnId);
    broadcastToRoom(room, passed);
    sendSpectateUpdatesToFinishedPlayers(room);
}

/// 机器人回合：出牌或过牌，并处理局终与下一计时
void MessageDispatcher::executeBotTurn(const std::shared_ptr<Room>& room, PlayerId botId) {
    if (!isBotPlayer(botId)) return;

    auto& engine = room->engine();
    const auto& state = engine.getState();
    // 非出牌阶段忽略定时触发
    if (state.phase != GamePhase::PLAYING) return;

    int seat = -1;
    for (int i = 0; i < state.playerCount; ++i) {
        if (state.players[i].id == botId) {
            seat = i;
            break;
        }
    }
    // 必须仍是当前行动玩家
    if (seat < 0 || seat != state.currentPlayerIndex) return;

    cancelTurnTimer(room->id());

    // 优先尝试引擎选牌出牌
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

    // 无法出牌则过牌
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

/// 校验是否处于 PLAYING 且为当前玩家回合（含 turnId）
bool MessageDispatcher::validateTurn(const std::shared_ptr<Room>& room, PlayerId playerId,
                                     const Message& msg, ErrorCode* rejectCode) {
    const auto& state = room->engine().getState();
    // 必须在 PLAYING 阶段
    if (state.phase != GamePhase::PLAYING) {
        if (rejectCode) *rejectCode = ErrorCode::INVALID_STATE;
        return false;
    }
    // 客户端 turnId 与服务器不一致视为过期请求
    if (msg.turnId > 0 && msg.turnId != state.turnId) {
        if (rejectCode) *rejectCode = ErrorCode::REQUEST_EXPIRED;
        return false;
    }
    // 当前行动座位必须有效
    if (state.currentPlayerIndex < 0 || state.currentPlayerIndex >= state.playerCount) {
        if (rejectCode) *rejectCode = ErrorCode::INVALID_STATE;
        return false;
    }
    // 必须轮到该玩家行动
    if (state.players[state.currentPlayerIndex].id != playerId) {
        if (rejectCode) *rejectCode = ErrorCode::NOT_YOUR_TURN;
        return false;
    }
    return true;
}

/// Token 登录：绑定玩家并返回是否在房及房间摘要
void MessageDispatcher::handleLogin(uint64_t sessionId, const Message& msg, SendFn send) {
    // 校验 Token 并加载用户
    auto user = authService_.validateToken(msg.token);
    if (!user) {
        sendError(send, msg.requestId, ErrorCode::UNAUTHORIZED);
        return;
    }

    // 绑定 session↔player 并记录 Token
    sessionManager_.ensureSession(sessionId);
    PlayerId playerId = sessionManager_.bindPlayer(sessionId, user->id, user->nickname);
    sessionManager_.authenticate(sessionId, msg.token);

    auto room = roomManager_.findRoomByPlayer(playerId);
    const PlayerJoinInfo profile = joinInfoFromUser(*user);

    // 组装 login_result 的 dataJson
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
    // 同步回复 login_result
    sendResponse(send, resp);

    // 仍在有效房间内则异步同步资料并补发状态/快照
    if (room && room->phase() != RoomPhase::FINISHED) {
        const auto roomPhase = room->phase();
        const RoomId roomId = room->id();
        room->taskQueue().post([this, room, playerId, roomPhase, roomId, profile, send]() {
            room->syncPlayerProfile(playerId, profile);
            // 对局/结算中优先补快照
            if (roomPhase == RoomPhase::PLAYING || roomPhase == RoomPhase::SETTLEMENT) {
                if (roomPhase == RoomPhase::SETTLEMENT) {
                    Message stateMsg;
                    stateMsg.type = "room_state";
                    stateMsg.roomId = room->id();
                    stateMsg.dataJson = buildRoomStateJson(*room);
                    sendResponse(send, stateMsg);
                }
                sendSnapshotToPlayer(room, playerId);
            } else {
                Message stateMsg;
                stateMsg.type = "room_state";
                stateMsg.roomId = room->id();
                stateMsg.dataJson = buildRoomStateJson(*room);
                sendResponse(send, stateMsg);
            }
            Logger::info("Player " + std::to_string(playerId) + " rejoined room " + roomId);
        });
    }
}

/// 创建房间并广播初始 room_state
void MessageDispatcher::handleCreateRoom(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    const PlayerJoinInfo profile =
        joinInfoFromSession(authService_, sessionManager_, sessionId);
    // 默认六人模式
    std::string mode = extractJsonStringFromData(msg.dataJson, "mode");
    if (mode.empty()) mode = "six";

    auto room = roomManager_.createRoom(playerId, profile, mode);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::ALREADY_IN_ROOM);
        return;
    }
    setupRoomBroadcast(room);

    // 请求方收到 room_created
    Message resp;
    resp.type = "room_created";
    resp.requestId = msg.requestId;
    resp.dataJson = "{\"room_id\":\"" + room->id() + "\",\"mode\":\"" + room->config().modeName +
                    "\",\"max_players\":" + std::to_string(room->config().maxPlayers) + "}";
    sendResponse(send, resp);

    // 异步广播完整 room_state
    room->taskQueue().post([this, room]() {
        Message stateMsg;
        stateMsg.type = "room_state";
        stateMsg.roomId = room->id();
        stateMsg.dataJson = buildRoomStateJson(*room);
        broadcastToRoom(room, stateMsg);
    });

    Logger::info("Room created: " + room->id() + " mode=" + room->config().modeName);
}

/// 加入指定房间并广播 player_joined 与 room_state
void MessageDispatcher::handleJoinRoom(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    const PlayerJoinInfo profile =
        joinInfoFromSession(authService_, sessionManager_, sessionId);
    const std::string& nickname = profile.nickname;

    auto room = roomManager_.joinRoom(msg.roomId, playerId, profile);
    if (!room) {
        // 区分房间不存在、已满、已在其他房
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

    // 加入者收到 room_joined（含 room_state）
    Message resp;
    resp.type = "room_joined";
    resp.requestId = msg.requestId;
    resp.roomId = msg.roomId;
    resp.dataJson = buildRoomStateJson(*room);
    sendResponse(send, resp);

    // 通知其他人有新玩家
    Message joinBroadcast;
    joinBroadcast.type = "player_joined";
    joinBroadcast.roomId = room->id();
    joinBroadcast.dataJson = "{\"player_id\":" + std::to_string(playerId) +
                             ",\"nickname\":\"" + nickname + "\"}";
    broadcastToRoom(room, joinBroadcast);

    // 全员同步最新 room_state
    Message stateMsg;
    stateMsg.type = "room_state";
    stateMsg.roomId = room->id();
    stateMsg.dataJson = buildRoomStateJson(*room);
    broadcastToRoom(room, stateMsg);
}

/// 离开房间并更新剩余玩家的 room_state
void MessageDispatcher::handleLeaveRoom(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    auto room = roomManager_.findRoomByPlayer(playerId);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::NOT_IN_ROOM);
        return;
    }
    RoomId roomId = room->id();
    // 从房间与全局索引移除玩家
    room->leave(playerId);
    roomManager_.removePlayerFromRoom(playerId);

    Message resp;
    resp.type = "player_left";
    resp.requestId = msg.requestId;
    resp.dataJson = "{\"player_id\":" + std::to_string(playerId) + "}";
    sendResponse(send, resp);

    // 若房间未空则广播更新后的 room_state
    auto remaining = roomManager_.getRoom(roomId);
    if (remaining && !remaining->isEmpty()) {
        Message stateMsg;
        stateMsg.type = "room_state";
        stateMsg.roomId = roomId;
        stateMsg.dataJson = buildRoomStateJson(*remaining);
        broadcastToRoom(remaining, stateMsg);
    }
}

/// 玩家准备；全员就绪时尝试自动开下一局
void MessageDispatcher::handleReady(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    auto room = roomManager_.findRoomByPlayer(playerId);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::NOT_IN_ROOM);
        return;
    }
    room->ready(playerId);

    // 广播 player_ready
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
    // 结算阶段全员 ready 时可自动开下一局
    tryStartNextRound(room);
}

/// 取消准备并广播 room_state
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

    // 广播 player_unready 与 room_state
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

/// 换座并广播 seat_changed 与 room_state
void MessageDispatcher::handleChangeSeat(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    auto room = roomManager_.findRoomByPlayer(playerId);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::NOT_IN_ROOM);
        return;
    }

    int seatIndex = static_cast<int>(extractJsonUintFromData(msg.dataJson, "seat_index"));
    // 房间层校验并执行换座
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

/// 广播 game_started 并进入单局开局流程
void MessageDispatcher::onGameStarted(const std::shared_ptr<Room>& room, uint64_t requestId) {
    // 全员收到 game_started
    Message started;
    started.type = "game_started";
    started.requestId = requestId;
    started.roomId = room->id();
    broadcastToRoom(room, started);
    onRoundStarted(room);
}

/// 发牌视图、处理贡牌定时/结果广播并启动出牌计时
void MessageDispatcher::onRoundStarted(const std::shared_ptr<Room>& room) {
    auto& engine = room->engine();
    const auto& state = engine.getState();

    // 向每位玩家下发发牌视图
    for (const auto& p : room->players()) {
        sendPlayerView(room, p.id, "cards_dealt");
    }

    // 仍有人类/机器人待贡则只开贡牌定时器
    if (engine.hasPendingTributeAction()) {
        scheduleTributeTimer(room);
        return;
    }

    // 若有贡牌结果需补广播 tribute_resolved
    const auto& tribute = engine.lastTributeResult();
    if (!tribute.skipped || tribute.antiTribute || !tribute.tributes.empty()) {
        Message tributeMsg;
        tributeMsg.type = "tribute_resolved";
        tributeMsg.roomId = room->id();
        tributeMsg.dataJson = buildTributeResolvedJson(tribute);
        broadcastToRoom(room, tributeMsg);
    }

    // 已进入出牌则启动回合计时
    if (state.phase == GamePhase::PLAYING) {
        scheduleTurnTimer(room);
    }
}

/// 局终结算广播；整场比赛结束或进入结算并尝试下一局
void MessageDispatcher::onGameOver(const std::shared_ptr<Room>& room,
                                     const SettlementResult& result) {
    // 对局结束停止一切房间定时器
    cancelTurnTimer(room->id());
    cancelTributeTimer(room->id());

    // 构建 settlement / game_over 共用 JSON
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
    // 附带每位玩家的终局名次
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

    // 整场比赛胜利则 game_over 并结束房间
    if (result.matchWon) {
        Message over;
        over.type = "game_over";
        over.roomId = room->id();
        over.dataJson = oss.str();
        broadcastToRoom(room, over);
        room->finishMatch();
        return;
    }

    // 单局结束进入结算，等待 ready 开下一局
    room->enterSettlement();
    Message stateMsg;
    stateMsg.type = "room_state";
    stateMsg.roomId = room->id();
    stateMsg.dataJson = buildRoomStateJson(*room);
    broadcastToRoom(room, stateMsg);
    tryStartNextRound(room);
}

/// 结算阶段全员准备后自动 startNextRound
void MessageDispatcher::tryStartNextRound(const std::shared_ptr<Room>& room) {
    // 仅结算阶段且全员 ready 才开下一局
    if (room->phase() != RoomPhase::SETTLEMENT || !room->allReady()) return;
    // 房间引擎开新一局
    if (!room->startNextRound()) return;
    onRoundStarted(room);
}

/// 房主修改房间选项（如是否启用贡牌）
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
    // 房间状态须允许修改贡牌开关
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

    // 广播最新 room_state
    Message stateMsg;
    stateMsg.type = "room_state";
    stateMsg.roomId = room->id();
    stateMsg.dataJson = buildRoomStateJson(*room);
    broadcastToRoom(room, stateMsg);
}

/// 房主开局：校验后广播并回复 game_started
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
    // 广播开局并走发牌/贡牌流程
    onGameStarted(room, msg.requestId);
    Message resp;
    resp.type = "game_started";
    resp.requestId = msg.requestId;
    resp.roomId = room->id();
    sendResponse(send, resp);
}

/// 人类出牌：回合校验、引擎出牌、广播与计时
void MessageDispatcher::handlePlayCards(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    auto room = roomManager_.findRoomByPlayer(playerId);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::NOT_IN_ROOM);
        return;
    }
    // 服务端权威：阶段、turn_id、当前出牌位
    ErrorCode turnReject = ErrorCode::REQUEST_EXPIRED;
    if (!validateTurn(room, playerId, msg, &turnReject)) {
        sendError(send, msg.requestId, turnReject);
        return;
    }
    if (msg.cards.empty()) {
        sendError(send, msg.requestId, ErrorCode::INVALID_CARDS);
        return;
    }

    // 牌型/压牌/手牌/轮转由 GameEngine 判定并改状态
    auto result = room->engine().playCards(playerId, msg.cards);
    if (result.code != ErrorCode::OK) {
        sendError(send, msg.requestId, result.code);
        return;
    }

    cancelTurnTimer(room->id());
    // 全员 player_played；若结束则 settlement
    broadcastPlayerPlayed(room, playerId, msg.cards, result);

    if (result.gameOver) {
        onGameOver(room, result.settlement);
    } else {
        scheduleTurnTimer(room);
    }

    // 请求方单独 ack（含最新 turn 信息）
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

/// 提交进贡牌并推进贡牌阶段/定时器
void MessageDispatcher::handleSubmitTribute(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    auto room = roomManager_.findRoomByPlayer(playerId);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::NOT_IN_ROOM);
        return;
    }

    CardId cardId = msg.cards.empty()
        ? static_cast<CardId>(extractJsonUintFromData(msg.dataJson, "card_id"))
        : msg.cards[0];
    if (cardId == 0) {
        sendError(send, msg.requestId, ErrorCode::INVALID_CARDS);
        return;
    }

    auto result = room->engine().submitTribute(playerId, cardId);
    if (result.code != ErrorCode::OK) {
        sendError(send, msg.requestId, result.code);
        return;
    }

    cancelTributeTimer(room->id());
    broadcastTributePhaseUpdate(room);

    // 贡牌未结束则续定时器，否则进入出牌
    auto& engine = room->engine();
    if (engine.hasPendingTributeAction()) {
        scheduleTributeTimer(room);
    } else if (engine.getState().phase == GamePhase::PLAYING) {
        onTributePhaseCompleted(room);
    }

    Message resp;
    resp.type = "submit_tribute";
    resp.requestId = msg.requestId;
    resp.roomId = room->id();
    sendResponse(send, resp);
}

/// 提交还贡牌；可能 finalize 并进入出牌
void MessageDispatcher::handleSubmitReturn(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    auto room = roomManager_.findRoomByPlayer(playerId);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::NOT_IN_ROOM);
        return;
    }

    CardId cardId = msg.cards.empty()
        ? static_cast<CardId>(extractJsonUintFromData(msg.dataJson, "card_id"))
        : msg.cards[0];
    if (cardId == 0) {
        sendError(send, msg.requestId, ErrorCode::INVALID_CARDS);
        return;
    }

    auto result = room->engine().submitReturn(playerId, cardId);
    if (result.code != ErrorCode::OK) {
        sendError(send, msg.requestId, result.code);
        return;
    }

    cancelTributeTimer(room->id());
    broadcastTributePhaseUpdate(room);

    // 还贡轮次结束或仍有待处理动作
    auto& engine = room->engine();
    if (result.tributeRoundComplete) {
        engine.finalizeTributePhase();
        onTributePhaseCompleted(room);
    } else if (engine.hasPendingTributeAction()) {
        scheduleTributeTimer(room);
    } else if (engine.getState().phase == GamePhase::PLAYING) {
        onTributePhaseCompleted(room);
    }

    Message resp;
    resp.type = "submit_return";
    resp.requestId = msg.requestId;
    resp.roomId = room->id();
    sendResponse(send, resp);
}

/// 过牌：校验回合、引擎 pass、广播与下一计时
void MessageDispatcher::handlePass(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    auto room = roomManager_.findRoomByPlayer(playerId);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::NOT_IN_ROOM);
        return;
    }
    ErrorCode turnReject = ErrorCode::REQUEST_EXPIRED;
    if (!validateTurn(room, playerId, msg, &turnReject)) {
        sendError(send, msg.requestId, turnReject);
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

    // 过牌 ack 给请求方
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

/// 已出完牌的玩家切换观战队友视角
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

    // 按新锚点构建观战快照并回复
    Message resp;
    resp.type = "spectate_changed";
    resp.requestId = msg.requestId;
    resp.roomId = room->id();
    const int anchor = room->resolveViewAnchor(playerId);
    auto view = room->engine().buildViewFor(playerId, anchor);
    resp.dataJson = buildGameSnapshotJson(view, *room);
    sendResponse(send, resp);
}

/// 重连确认并下发快照或 room_state
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

    // 对局中补 game_snapshot，否则 room_state
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

/// 座位聊天：校验内容长度后房间广播
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

    // 房间广播 seat_chat
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

/// 桌内表情：校验目标与频控后广播 emotion_event
void MessageDispatcher::handleSendEmotion(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId senderId = resolvePlayerId(sessionId);
    auto room = roomManager_.findRoomByPlayer(senderId);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::NOT_IN_ROOM);
        return;
    }

    const PlayerId targetId =
        static_cast<PlayerId>(extractJsonUintFromData(msg.dataJson, "target_player_id"));
    const int emotionType =
        static_cast<int>(extractJsonUintFromData(msg.dataJson, "emotion_type"));

    // 参数与业务规则校验（失败返回 emotion_error）
    if (targetId == 0) {
        Message resp;
        resp.type = "emotion_error";
        resp.requestId = msg.requestId;
        resp.roomId = room->id();
        resp.dataJson = "{\"code\":\"INVALID_EMOTION_TYPE\"}";
        sendResponse(send, resp);
        return;
    }

    if (!isValidEmotionType(emotionType)) {
        Message resp;
        resp.type = "emotion_error";
        resp.requestId = msg.requestId;
        resp.roomId = room->id();
        resp.dataJson = "{\"code\":\"INVALID_EMOTION_TYPE\"}";
        sendResponse(send, resp);
        return;
    }

    if (senderId == targetId) {
        Message resp;
        resp.type = "emotion_error";
        resp.requestId = msg.requestId;
        resp.roomId = room->id();
        resp.dataJson = "{\"code\":\"CANNOT_SEND_TO_SELF\"}";
        sendResponse(send, resp);
        return;
    }

    if (!room->containsPlayer(targetId)) {
        Message resp;
        resp.type = "emotion_error";
        resp.requestId = msg.requestId;
        resp.roomId = room->id();
        resp.dataJson = "{\"code\":\"TARGET_NOT_IN_ROOM\"}";
        sendResponse(send, resp);
        return;
    }

    if (!emotionRateLimiter_.tryAcquire(senderId)) {
        Message resp;
        resp.type = "emotion_error";
        resp.requestId = msg.requestId;
        resp.roomId = room->id();
        resp.dataJson = "{\"code\":\"TOO_FREQUENT\"}";
        sendResponse(send, resp);
        return;
    }

    const uint64_t eventId = room->nextEmotionEventId();
    const int64_t serverTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // 全员 emotion_event
    Message broadcast;
    broadcast.type = "emotion_event";
    broadcast.roomId = room->id();
    broadcast.dataJson = std::string("{\"event_id\":") + std::to_string(eventId) +
                         ",\"sender_id\":" + std::to_string(senderId) +
                         ",\"target_id\":" + std::to_string(targetId) +
                         ",\"emotion_type\":" + std::to_string(emotionType) +
                         ",\"server_time\":" + std::to_string(serverTimeMs) + "}";
    broadcastToRoom(room, broadcast);

    // 发送方 ack（内容与广播一致）
    Message resp;
    resp.type = "send_emotion_ack";
    resp.requestId = msg.requestId;
    resp.roomId = room->id();
    resp.dataJson = broadcast.dataJson;
    sendResponse(send, resp);
}

/// 广播玩家正在说话状态（游戏服侧）
void MessageDispatcher::handleVoiceSpeaking(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    auto room = roomManager_.findRoomByPlayer(playerId);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::NOT_IN_ROOM);
        return;
    }

    const bool speaking = extractJsonBoolFromData(msg.dataJson, "speaking");

    // 广播 speaking 指示
    Message broadcast;
    broadcast.type = "voice_speaking";
    broadcast.roomId = room->id();
    broadcast.dataJson = std::string("{\"player_id\":") + std::to_string(playerId) +
                         ",\"speaking\":" + (speaking ? "true" : "false") + "}";
    broadcastToRoom(room, broadcast);

    Message resp;
    resp.type = "voice_speaking_ack";
    resp.requestId = msg.requestId;
    resp.roomId = room->id();
    sendResponse(send, resp);
}

/// 广播麦克风/扬声器开关状态
void MessageDispatcher::handleVoiceState(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    auto room = roomManager_.findRoomByPlayer(playerId);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::NOT_IN_ROOM);
        return;
    }

    const bool micEnabled = extractJsonBoolFromData(msg.dataJson, "mic_enabled");
    const bool speakerEnabled = extractJsonBoolFromData(msg.dataJson, "speaker_enabled");

    // 广播 voice_state
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

/// 转发 WebRTC Mesh 信令（全员或指定玩家）
void MessageDispatcher::handleVoiceSignal(uint64_t sessionId, const Message& msg, SendFn send) {
    PlayerId playerId = resolvePlayerId(sessionId);
    auto room = roomManager_.findRoomByPlayer(playerId);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::NOT_IN_ROOM);
        return;
    }

    const PlayerId toPlayerId =
        static_cast<PlayerId>(extractJsonUintFromData(msg.dataJson, "to_player_id"));
    const std::string signal = extractJsonStringFromData(msg.dataJson, "signal");
    if (signal.empty()) {
        sendError(send, msg.requestId, ErrorCode::INVALID_MESSAGE);
        return;
    }

    // 解析可选 WebRTC 字段
    const std::string sdp = extractJsonStringFromData(msg.dataJson, "sdp");
    const std::string sdpType = extractJsonStringFromData(msg.dataJson, "sdp_type");
    const std::string candidate = extractJsonStringFromData(msg.dataJson, "candidate");
    const std::string sdpMid = extractJsonStringFromData(msg.dataJson, "sdp_mid");
    const int sdpMLineIndex =
        static_cast<int>(extractJsonUintFromData(msg.dataJson, "sdp_mline_index"));

    // 组装转发 payload（按需附带 SDP/ICE 字段）
    std::ostringstream data;
    data << "{\"from_player_id\":" << playerId << ",\"signal\":\"" << jsonEscape(signal)
         << "\"";
    if (!sdp.empty()) {
        data << ",\"sdp\":\"" << jsonEscape(sdp) << "\"";
    }
    if (!sdpType.empty()) {
        data << ",\"sdp_type\":\"" << jsonEscape(sdpType) << "\"";
    }
    if (!candidate.empty()) {
        data << ",\"candidate\":\"" << jsonEscape(candidate) << "\"";
    }
    if (!sdpMid.empty()) {
        data << ",\"sdp_mid\":\"" << jsonEscape(sdpMid) << "\"";
    }
    if (msg.dataJson.find("\"sdp_mline_index\"") != std::string::npos) {
        data << ",\"sdp_mline_index\":" << sdpMLineIndex;
    }
    data << "}";

    Message forward;
    forward.type = "voice_signal";
    forward.roomId = room->id();
    forward.dataJson = data.str();

    if (toPlayerId == 0) {
        // 广播给除发送者外的所有玩家
        for (const auto& p : room->players()) {
            if (p.id == playerId) {
                continue;
            }
            sessionManager_.sendToPlayer(p.id, messageToJson(forward));
        }
    } else {
        bool inRoom = false;
        for (const auto& p : room->players()) {
            if (p.id == toPlayerId) {
                inRoom = true;
                break;
            }
        }
        if (!inRoom || toPlayerId == playerId) {
            sendError(send, msg.requestId, ErrorCode::INVALID_STATE);
            return;
        }
        sessionManager_.sendToPlayer(toPlayerId, messageToJson(forward));
    }

    Message resp;
    resp.type = "voice_signal_ack";
    resp.requestId = msg.requestId;
    resp.roomId = room->id();
    sendResponse(send, resp);
}

/// 签发 LiveKit 房间 Token 供客户端进房
void MessageDispatcher::handleVoiceJoin(uint64_t sessionId, const Message& msg, SendFn send) {
    if (!liveKit_.enabled) {
        sendError(send, msg.requestId, ErrorCode::INVALID_STATE);
        return;
    }

    PlayerId playerId = resolvePlayerId(sessionId);
    auto room = roomManager_.findRoomByPlayer(playerId);
    if (!room) {
        sendError(send, msg.requestId, ErrorCode::NOT_IN_ROOM);
        return;
    }

    // 签发 Token 用的展示名
    std::string nickname = "Player";
    for (const auto& p : room->players()) {
        if (p.id == playerId) {
            nickname = p.nickname.empty() ? nickname : p.nickname;
            break;
        }
    }

    const std::string identity = std::to_string(playerId);
    const std::string token =
        createLiveKitAccessToken(liveKit_, room->id(), identity, nickname);
    if (token.empty()) {
        sendError(send, msg.requestId, ErrorCode::INTERNAL_ERROR);
        return;
    }

    // 返回 token 与 LiveKit 房间名
    Message resp;
    resp.type = "voice_join_ok";
    resp.requestId = msg.requestId;
    resp.roomId = room->id();
    resp.dataJson = std::string("{\"token\":\"") + jsonEscape(token) + "\",\"room_name\":\"" +
                    jsonEscape(room->id()) + "\"}";
    sendResponse(send, resp);
    Logger::info("LiveKit voice_join for player " + std::to_string(playerId) + " room " +
                 room->id());
}

/// 心跳：回复 pong
void MessageDispatcher::handlePing(SendFn send) {
    // 心跳应答，无 requestId
    Message msg;
    msg.type = "pong";
    sendResponse(send, msg);
}

/// 广播当前解散投票状态
void MessageDispatcher::broadcastDismissVote(const std::shared_ptr<Room>& room,
                                             const std::string& type) {
    // type 如 dismiss_vote_started / dismiss_vote_updated
    Message msg;
    msg.type = type;
    msg.roomId = room->id();
    msg.dataJson = buildDismissVoteJson(room->dismissVote());
    broadcastToRoom(room, msg);
}

/// 取消计时、通知解散并从 RoomManager 移除
void MessageDispatcher::dissolveRoom(const std::shared_ptr<Room>& room) {
    cancelTurnTimer(room->id());
    const RoomId roomId = room->id();

    // 通知全员房间已解散
    Message msg;
    msg.type = "room_dismissed";
    msg.roomId = roomId;
    msg.dataJson = "{}";
    broadcastToRoom(room, msg);

    roomManager_.removeRoom(roomId);
    Logger::info("Room dissolved: " + roomId);
}

/// 发起房间解散投票
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

    // 发起人收到带投票快照的 ack
    Message resp;
    resp.type = "dismiss_vote_started";
    resp.requestId = msg.requestId;
    resp.roomId = room->id();
    resp.dataJson = buildDismissVoteJson(room->dismissVote());
    sendResponse(send, resp);

    // 仅一人房等场景可能立即全票通过
    if (room->dismissVote().allAgreed()) {
        dissolveRoom(room);
    }
}

/// 对解散投票投同意/拒绝并处理结果
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

    // 投票者收到当前票型 ack
    Message resp;
    resp.type = agree ? "dismiss_vote_updated" : "dismiss_vote_rejected";
    resp.requestId = msg.requestId;
    resp.roomId = room->id();
    resp.dataJson = buildDismissVoteJson(room->dismissVote());
    sendResponse(send, resp);

    if (!agree || room->dismissVote().hasRejection()) {
        // 有人拒绝则取消投票并广播 rejected
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

    // 全票同意则解散
    if (room->dismissVote().allAgreed()) {
        dissolveRoom(room);
    }
}

/// 解析 JSON 并按类型投递到房间队列或直接 dispatchMessage
void MessageDispatcher::dispatch(uint64_t sessionId, const std::string& rawJson, SendFn send) {
    Message msg;
    try {
        // 解析 WebSocket 原始 JSON
        msg = parseMessage(rawJson);
    } catch (...) {
        // JSON 或字段不合法
        sendError(send, 0, ErrorCode::INVALID_MESSAGE);
        return;
    }

    // login/ping/create_room 不走房间串行队列
    if (!shouldRunOnRoomQueue(msg.type)) {
        dispatchMessage(sessionId, msg, send);
        return;
    }

    postRoomMessage(sessionId, msg, send);
}

/// 判断是否需要在房间 taskQueue 上串行处理
bool MessageDispatcher::shouldRunOnRoomQueue(const std::string& type) const {
    // 上述三类可在连接线程直接处理
    return type != "login" && type != "ping" && type != "create_room";
}

/// 从 msg.roomId 或当前玩家所在房间解析 Room
std::shared_ptr<Room> MessageDispatcher::resolveRoomForDispatch(uint64_t sessionId,
                                                                const Message& msg) {
    if (!msg.roomId.empty()) {
        if (auto room = roomManager_.getRoom(msg.roomId)) {
            return room;
        }
    }
    // 回退：按 session 绑定玩家查所在房间
    PlayerId playerId = resolvePlayerId(sessionId);
    if (playerId > 0) {
        return roomManager_.findRoomByPlayer(playerId);
    }
    return nullptr;
}

/// 将消息 post 到房间队列；无法定位房间时直接处理
void MessageDispatcher::postRoomMessage(uint64_t sessionId, const Message& msg, SendFn send) {
    std::shared_ptr<Room> room = resolveRoomForDispatch(sessionId, msg);
    // join_room 时尚未入房，仅按 roomId 查找
    if (!room && msg.type == "join_room" && !msg.roomId.empty()) {
        room = roomManager_.getRoom(msg.roomId);
    }

    if (!room) {
        // 无房间上下文则当前线程直接处理
        dispatchMessage(sessionId, msg, send);
        return;
    }

    // 串行化房间相关消息
    room->taskQueue().post([this, room, sessionId, msg, send]() {
        dispatchMessage(sessionId, msg, send);
    });
}

/// 按消息 type 分发到具体 handler（通常已在房间任务队列内执行）
void MessageDispatcher::dispatchMessage(uint64_t sessionId, const Message& msg, SendFn send) {
    // 按 type 路由到对应业务 handler
    if (msg.type == "login") {
        handleLogin(sessionId, msg, send);  // Token 登录并绑定 Session
    } else if (msg.type == "create_room") {
        handleCreateRoom(sessionId, msg, send);  // 创建房间并广播 room_state
    } else if (msg.type == "join_room") {
        handleJoinRoom(sessionId, msg, send);  // 加入指定房间
    } else if (msg.type == "leave_room") {
        handleLeaveRoom(sessionId, msg, send);  // 离开房间
    } else if (msg.type == "ready") {
        handleReady(sessionId, msg, send);  // 举手准备
    } else if (msg.type == "unready") {
        handleUnready(sessionId, msg, send);  // 取消准备
    } else if (msg.type == "change_seat") {
        handleChangeSeat(sessionId, msg, send);  // 换座位
    } else if (msg.type == "set_room_options") {
        handleSetRoomOptions(sessionId, msg, send);  // 房主修改房间选项（如贡牌）
    } else if (msg.type == "start_game") {
        handleStartGame(sessionId, msg, send);  // 房主开始游戏
    } else if (msg.type == "play_cards") {
        handlePlayCards(sessionId, msg, send);  // 出牌（回合校验 + 引擎规则）
    } else if (msg.type == "pass") {
        handlePass(sessionId, msg, send);  // 过牌
    } else if (msg.type == "submit_tribute") {
        handleSubmitTribute(sessionId, msg, send);  // 提交进贡牌
    } else if (msg.type == "submit_return") {
        handleSubmitReturn(sessionId, msg, send);  // 提交还贡牌
    } else if (msg.type == "spectate_teammate") {
        handleSpectateTeammate(sessionId, msg, send);  // 切换观战队友视角
    } else if (msg.type == "reconnect") {
        handleReconnect(sessionId, msg, send);  // 重连并下发快照或 room_state
    } else if (msg.type == "request_dismiss") {
        handleRequestDismiss(sessionId, msg, send);  // 发起解散投票
    } else if (msg.type == "vote_dismiss") {
        handleVoteDismiss(sessionId, msg, send);  // 投票同意/拒绝解散
    } else if (msg.type == "voice_state") {
        handleVoiceState(sessionId, msg, send);  // 开麦/闭麦状态广播
    } else if (msg.type == "seat_chat") {
        handleSeatChat(sessionId, msg, send);  // 座位聊天
    } else if (msg.type == "send_emotion") {
        handleSendEmotion(sessionId, msg, send);  // 桌内表情
    } else if (msg.type == "voice_speaking") {
        handleVoiceSpeaking(sessionId, msg, send);  // 正在说话指示（游戏服广播）
    } else if (msg.type == "voice_join") {
        handleVoiceJoin(sessionId, msg, send);  // 申请 LiveKit 语音 Token
    } else if (msg.type == "voice_signal") {
        handleVoiceSignal(sessionId, msg, send);  // WebRTC 信令（Mesh 遗留）
    } else if (msg.type == "ping") {
        handlePing(send);  // 心跳
    } else {
        sendError(send, msg.requestId, ErrorCode::UNKNOWN_TYPE);  // 未知 type
    }
}

}  // namespace guandan
