#pragma once

#include "game/game_engine.h"
#include "game/room_config.h"
#include "room/dismiss_vote.h"
#include "room/room_task_queue.h"
#include "game/types.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace guandan {

class RoomExecutor;

/// 房间生命周期阶段
enum class RoomPhase {
    CREATED,     ///< 已创建
    WAITING,     ///< 等待玩家与准备
    PLAYING,     ///< 对局进行中
    SETTLEMENT,  ///< 结算展示
    FINISHED     ///< 整场结束
};

/// 玩家加入/资料同步时携带的展示信息
struct PlayerJoinInfo {
    std::string nickname;      ///< 昵称
    std::string avatar;        ///< 头像 URL 或路径
    std::string avatarPreset;  ///< 内置头像预设 id
};

/// 房间内一名玩家的状态
struct RoomPlayer {
    PlayerId id = 0;                          ///< 玩家 id
    std::string nickname;                     ///< 昵称
    std::string avatar;                       ///< 头像
    std::string avatarPreset;                 ///< 头像预设
    int seatIndex = -1;                       ///< 座位号，-1 表示未入座
    bool isReady = false;                     ///< 是否已准备
    bool isOwner = false;                     ///< 是否为房主
    bool isBot = false;                       ///< 是否为机器人
    PlayerStatus status = PlayerStatus::ONLINE; ///< 在线状态
};

/// 游戏房间：玩家列表、对局引擎、解散投票与观战等
class Room {
public:
    Room(RoomId roomId, PlayerId ownerId, RoomConfig config, RoomExecutor& executor);

    /// 房间 id
    RoomId id() const { return roomId_; }
    /// 当前房间阶段
    RoomPhase phase() const { return phase_; }
    /// 房间配置（人数、模式等）
    const RoomConfig& config() const { return config_; }
    /// 房间内玩家列表
    const std::vector<RoomPlayer>& players() const { return players_; }
    /// 对局引擎
    GameEngine& engine() { return engine_; }
    /// 本房间串行任务队列
    RoomTaskQueue& taskQueue() { return *taskQueue_; }

    /// 玩家加入房间
    bool join(PlayerId playerId, const PlayerJoinInfo& info);
    /// 同步玩家昵称/头像等资料
    bool syncPlayerProfile(PlayerId playerId, const PlayerJoinInfo& info);
    /// 玩家离开
    void leave(PlayerId playerId);
    /// 标记准备
    bool ready(PlayerId playerId);
    /// 取消准备
    bool unready(PlayerId playerId);
    /// 换座
    bool changeSeat(PlayerId playerId, int seatIndex);
    /// 房主设置是否开启进贡
    bool setEnableTribute(PlayerId playerId, bool enabled);
    /// 开始游戏
    bool startGame();
    /// 开始下一局
    bool startNextRound();
    /// 进入结算阶段
    void enterSettlement();
    /// 是否全员已准备
    bool allReady() const;
    /// 整场对局结束
    void finishMatch();
    /// 空位填充机器人
    void fillBots();
    /// 是否已满员
    bool isFull() const { return static_cast<int>(players_.size()) >= config_.maxPlayers; }
    /// 是否无玩家
    bool isEmpty() const { return players_.empty(); }
    /// 房主 id
    PlayerId ownerId() const { return ownerId_; }

    /// 发起解散投票
    bool requestDismiss(PlayerId playerId);
    /// 对解散投票表态
    bool voteDismiss(PlayerId playerId, bool agree);
    /// 取消进行中的解散投票
    void cancelDismissVote();
    /// 当前解散投票状态
    const DismissVote& dismissVote() const { return dismissVote_; }
    /// 真人玩家的投票条目（不含机器人）
    std::vector<DismissVoteEntry> humanVoteEntries() const;

    /// 第一个空座位号，无则返回 -1
    int firstEmptySeat() const;
    /// 座位是否已被占用（可排除 exceptId）
    bool isSeatTaken(int seatIndex, PlayerId exceptId = 0) const;

    /// 清空所有观战目标
    void clearSpectateTargets();
    /// 设置观战某座位队友
    bool setSpectateTarget(PlayerId viewerId, int targetSeat);
    /// 查询观战目标座位，-1 表示未观战
    int spectateTargetFor(PlayerId viewerId) const;
    /// 解析玩家视角锚点座位（本人或观战对象）
    int resolveViewAnchor(PlayerId viewerId) const;
    /// 可观战的队友座位列表
    std::vector<int> spectatableTeammateSeats(PlayerId viewerId) const;

    /// 是否包含该玩家
    bool containsPlayer(PlayerId playerId) const;
    /// 生成下一个表情事件 id
    uint64_t nextEmotionEventId();

    /// 向房间内玩家广播消息的回调
    using BroadcastFn = std::function<void(PlayerId, const std::string&)>;
    /// 设置广播回调
    void setBroadcastCallback(BroadcastFn fn) { broadcast_ = std::move(fn); }

private:
    /// 在指定座位添加机器人
    bool addBotAtSeat(int seatIndex, int botIndex);

    RoomId roomId_;                                      ///< 房间 id
    PlayerId ownerId_;                                   ///< 房主 id
    RoomConfig config_;                                  ///< 房间配置
    RoomPhase phase_ = RoomPhase::WAITING;               ///< 当前阶段
    std::vector<RoomPlayer> players_;                    ///< 玩家列表
    GameEngine engine_;                                  ///< 对局引擎
    std::shared_ptr<RoomTaskQueue> taskQueue_;           ///< 房间任务队列
    BroadcastFn broadcast_;                              ///< 广播回调
    DismissVote dismissVote_;                            ///< 解散投票
    std::unordered_map<PlayerId, int> spectateTargets_;  ///< 观战目标座位
    uint64_t emotionEventId_ = 0;                        ///< 表情事件序号
};

}  // namespace guandan
