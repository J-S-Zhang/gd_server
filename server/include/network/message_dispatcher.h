#pragma once

#include "game/game_engine.h"
#include "game/settlement.h"
#include "protocol/message.h"
#include "auth/auth_service.h"
#include "network/session_manager.h"
#include "room/emotion.h"
#include "room/room_manager.h"
#include "timer/timer_manager.h"
#include "livekit/livekit_config.h"
#include <functional>
#include <memory>
#include <string>

namespace guandan {

using SendFn = std::function<void(const std::string&)>;  ///< 向当前会话发送 JSON

/// WebSocket 消息路由：登录、房间、对局、语音、表情等
class MessageDispatcher {
public:
    MessageDispatcher(RoomManager& roomManager, SessionManager& sessionManager,
                      TimerManager& timerManager, AuthService& authService);

    /// 解析 rawJson 并分发处理
    void dispatch(uint64_t sessionId, const std::string& rawJson, SendFn send);
    /// 为房间注册广播回调
    void setupRoomBroadcast(const std::shared_ptr<Room>& room);
    /// 设置 LiveKit 配置
    void setLiveKitConfig(const LiveKitConfig& config);

private:
    RoomManager& roomManager_;           ///< 房间管理
    SessionManager& sessionManager_;     ///< 会话
    TimerManager& timerManager_;         ///< 出牌/进贡定时器
    AuthService& authService_;           ///< 认证
    int turnTimeoutSeconds_ = 30;        ///< 出牌超时
    EmotionRateLimiter emotionRateLimiter_;  ///< 表情限流
    LiveKitConfig liveKit_;              ///< LiveKit

    /// 从 session 解析 playerId
    PlayerId resolvePlayerId(uint64_t sessionId);
    /// 向房间内所有玩家广播
    void broadcastToRoom(const std::shared_ptr<Room>& room, const Message& msg);
    /// 向玩家发送完整快照
    void sendSnapshotToPlayer(const std::shared_ptr<Room>& room, PlayerId playerId);
    /// 发送玩家视角视图（指定 messageType 包装）
    void sendPlayerView(const std::shared_ptr<Room>& room, PlayerId playerId,
                        const std::string& messageType);
    /// 已向出完牌玩家推送观战视角更新
    void sendSpectateUpdatesToFinishedPlayers(const std::shared_ptr<Room>& room);
    /// 调度出牌超时定时器
    void scheduleTurnTimer(const std::shared_ptr<Room>& room);
    /// 取消出牌定时器
    void cancelTurnTimer(const RoomId& roomId);
    /// 调度进贡超时
    void scheduleTributeTimer(const std::shared_ptr<Room>& room);
    /// 取消进贡定时器
    void cancelTributeTimer(const RoomId& roomId);
    /// 机器人自动进贡/还贡
    void executeBotTributeActions(const std::shared_ptr<Room>& room);
    /// 广播进贡阶段状态
    void broadcastTributePhaseUpdate(const std::shared_ptr<Room>& room);
    /// 进贡阶段完成后续处理
    void onTributePhaseCompleted(const std::shared_ptr<Room>& room);
    /// 尝试自动开下一局
    void tryStartNextRound(const std::shared_ptr<Room>& room);
    /// 机器人出牌
    void executeBotTurn(const std::shared_ptr<Room>& room, PlayerId botId);
    /// 广播出牌事件
    void broadcastPlayerPlayed(const std::shared_ptr<Room>& room, PlayerId playerId,
                               const std::vector<CardId>& cards, const PlayResult& result);
    /// 广播过牌事件
    void broadcastPlayerPassed(const std::shared_ptr<Room>& room, PlayerId playerId,
                               const PlayResult& result);

    void handleLogin(uint64_t sessionId, const Message& msg, SendFn send);
    void handleCreateRoom(uint64_t sessionId, const Message& msg, SendFn send);
    void handleJoinRoom(uint64_t sessionId, const Message& msg, SendFn send);
    void handleLeaveRoom(uint64_t sessionId, const Message& msg, SendFn send);
    void handleReady(uint64_t sessionId, const Message& msg, SendFn send);
    void handleUnready(uint64_t sessionId, const Message& msg, SendFn send);
    void handleChangeSeat(uint64_t sessionId, const Message& msg, SendFn send);
    void handleSetRoomOptions(uint64_t sessionId, const Message& msg, SendFn send);
    void handleStartGame(uint64_t sessionId, const Message& msg, SendFn send);
    void handlePlayCards(uint64_t sessionId, const Message& msg, SendFn send);
    void handlePass(uint64_t sessionId, const Message& msg, SendFn send);
    void handleSubmitTribute(uint64_t sessionId, const Message& msg, SendFn send);
    void handleSubmitReturn(uint64_t sessionId, const Message& msg, SendFn send);
    void handleSpectateTeammate(uint64_t sessionId, const Message& msg, SendFn send);
    void handleReconnect(uint64_t sessionId, const Message& msg, SendFn send);
    void handleRequestDismiss(uint64_t sessionId, const Message& msg, SendFn send);
    void handleVoteDismiss(uint64_t sessionId, const Message& msg, SendFn send);
    void handleVoiceState(uint64_t sessionId, const Message& msg, SendFn send);
    void handleSeatChat(uint64_t sessionId, const Message& msg, SendFn send);
    void handleSendEmotion(uint64_t sessionId, const Message& msg, SendFn send);
    void handleVoiceSpeaking(uint64_t sessionId, const Message& msg, SendFn send);
    void handleVoiceJoin(uint64_t sessionId, const Message& msg, SendFn send);
    void handleVoiceSignal(uint64_t sessionId, const Message& msg, SendFn send);
    void handlePing(SendFn send);

    /// 已解析消息的统一入口
    void dispatchMessage(uint64_t sessionId, const Message& msg, SendFn send);
    /// 解析消息关联的房间
    std::shared_ptr<Room> resolveRoomForDispatch(uint64_t sessionId, const Message& msg);
    /// 该类型是否必须在房间队列串行执行
    bool shouldRunOnRoomQueue(const std::string& type) const;
    /// 投递到房间 taskQueue 执行
    void postRoomMessage(uint64_t sessionId, const Message& msg, SendFn send);

    /// 校验 turnId/stateVersion 与当前出牌权
    bool validateTurn(const std::shared_ptr<Room>& room, PlayerId playerId, const Message& msg,
                      ErrorCode* rejectCode = nullptr);

    void sendError(SendFn send, uint64_t requestId, ErrorCode code);
    void sendResponse(SendFn send, const Message& msg);
    void onGameStarted(const std::shared_ptr<Room>& room, uint64_t requestId);
    void onRoundStarted(const std::shared_ptr<Room>& room);
    void onGameOver(const std::shared_ptr<Room>& room, const SettlementResult& result);
    void broadcastDismissVote(const std::shared_ptr<Room>& room, const std::string& type);
    void dissolveRoom(const std::shared_ptr<Room>& room);
};

}  // namespace guandan
