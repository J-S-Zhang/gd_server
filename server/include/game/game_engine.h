#pragma once

#include "game/card.h"
#include "game/bot_player.h"
#include "game/card_analyzer.h"
#include "game/game_state.h"
#include "game/rule_engine.h"
#include "game/settlement.h"
#include "game/team_progress.h"
#include "game/tribute.h"
#include "game/turn_manager.h"
#include "game/types.h"
#include <array>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace guandan {

/// 单次 play/pass/进贡等操作的统一结果
struct PlayResult {
    ErrorCode code = ErrorCode::OK;  ///< 错误码
    std::string message;             ///< 说明
    bool gameOver = false;           ///< 是否整场结束
    bool tributeRoundComplete = false;  ///< 进贡交互阶段是否刚完成
    SettlementResult settlement;     ///< 若局/场结束则填充
};

/// 发给单个玩家的对局视图（含手牌与他人摘要）
struct PlayerView {
    GamePhase phase;                      ///< 阶段
    uint64_t stateVersion;                ///< 状态版本
    uint64_t turnId;                      ///< 回合 id
    int currentLevel;                     ///< 当前级牌
    int attackingTeam;                    ///< 攻方
    bool isPassARound;                    ///< 是否过 A 局
    bool isPlayingOwnRound;               ///< 本队是否攻方
    std::array<int, 2> teamLevels{2, 2};  ///< 两队级牌
    std::array<bool, 2> inPassAPhase{false, false};
    std::array<int, 2> passAFailCounts{0, 0};
    int currentPlayerIndex;               ///< 当前出牌座位
    int firstPlayerIndex;                 ///< 本局首出
    int mySeatIndex;                      ///< 视角玩家座位
    int viewerSeatIndex = -1;            ///< 实际视角锚点（观战时为队友）
    bool isSpectating = false;            ///< 是否在观战
    std::vector<int> spectatableTeammates;  ///< 可观战队友座位
    std::vector<CardId> myCards;          ///< 手牌（仅本人可见）
    std::vector<CardId> lastPlayedCards;  ///< 上一手牌
    int lastPlayedPlayerIndex;            ///< 上一手出牌座位
    CardPattern lastPattern;              ///< 上一手牌型

    std::vector<int> pendingTributerSeats;   ///< 待进贡座位
    std::vector<int> pendingReturnSeats;     ///< 待还贡座位
    CardId requiredTributeCardId = 0;        ///< 进贡必须出的牌（若规则强制）
    std::vector<CardId> validReturnCardIds;  ///< 可还贡牌列表
    bool mustReturnTribute = false;          ///< 当前是否必须还贡

    /// 进贡/还贡展示项
    struct TributeSeatPlay {
        int seatIndex = -1;   ///< 座位
        CardId cardId = 0;    ///< 牌 id
        std::string kind;     ///< tribute/return 等
    };
    std::vector<TributeSeatPlay> tributeSeatPlays;

    /// 其他玩家公开信息
    struct OtherPlayer {
        PlayerId id;              ///< 玩家 id
        int seatIndex;              ///< 座位
        int team;                   ///< 队伍
        int cardCount;              ///< 剩余牌数
        bool hasFinished;           ///< 是否出完
        int finishRank;             ///< 游次
        bool isReady;               ///< 准备状态
        PlayerStatus status;        ///< 在线状态
    };
    std::vector<OtherPlayer> others;
};

/// 对局核心：状态机、出牌、进贡、机器人辅助
class GameEngine {
public:
    explicit GameEngine(const GameRuleConfig& config = {});

    /// 初始化房间与玩家 id 列表
    void init(RoomId roomId, const std::vector<PlayerId>& playerIds);
    /// 设置玩家准备（默认 true）
    void setPlayerReady(PlayerId playerId);
    void setPlayerReady(PlayerId playerId, bool ready);
    /// 是否全员准备
    bool allReady() const;
    /// 更新规则配置
    void updateConfig(const GameRuleConfig& config);
    /// 开始第一局
    PlayResult startGame();
    /// 开始下一局
    PlayResult startNextRound();

    /// 出牌
    PlayResult playCards(PlayerId playerId, const std::vector<CardId>& cards);
    /// 过牌
    PlayResult pass(PlayerId playerId);
    /// 提交进贡牌
    PlayResult submitTribute(PlayerId playerId, CardId cardId);
    /// 提交还贡牌
    PlayResult submitReturn(PlayerId playerId, CardId cardId);
    /// 强制结束进贡阶段（如超时）
    PlayResult finalizeTributePhase();

    /// 机器人选牌出牌
    std::optional<std::vector<CardId>> chooseBotPlay(PlayerId playerId) const;
    std::optional<CardId> chooseBotTributeCard(PlayerId playerId) const;
    std::optional<CardId> chooseBotReturnCard(PlayerId playerId) const;
    /// 是否有待处理的进贡/还贡
    bool hasPendingTributeAction() const;
    /// 待进贡座位列表
    const std::vector<int>& pendingTributerSeats() const {
        return tributeState_.pendingTributers;
    }
    /// 待还贡座位列表
    const std::vector<int>& pendingReturnSeats() const {
        return tributeState_.pendingReturnSeats;
    }

    /// 整场是否已 FINISHED
    bool isGameOver() const { return state_.phase == GamePhase::FINISHED; }
    const GameState& getState() const { return state_; }
    const TeamProgress& teamProgress() const { return progress_; }
    const TributeRoundResult& lastTributeResult() const { return lastTribute_; }
    /// 构建玩家视角；anchorSeatOverride 用于观战
    PlayerView buildViewFor(PlayerId viewerId, int anchorSeatOverride = -1) const;

private:
    GameRuleConfig config_;       ///< 规则配置
    GameState state_;             ///< 对局状态
    CardAnalyzer analyzer_;       ///< 牌型分析
    RuleEngine ruleEngine_;       ///< 压牌规则
    TurnManager turnManager_;     ///< 轮转
    Settlement settlement_;       ///< 结算
    TeamProgress progress_;       ///< 跨局进度
    TributeManager tributeManager_;  ///< 进贡规则
    BotPlayer botPlayer_;         ///< 机器人
    PreviousRoundInfo previousRound_{};  ///< 上一局信息（进贡用）
    TributeRoundResult lastTribute_{};   ///< 最近一次进贡结果

    /// 交互式进贡中间状态
    struct InteractiveTributeState {
        bool active = false;                       ///< 是否在交互进贡中
        TributePhasePlan plan;                     ///< 计划
        std::vector<int> pendingTributers;         ///< 未提交进贡的座位
        std::map<int, CardId> tributeSubmissions;  ///< 已提交进贡
        std::vector<int> pendingReturnSeats;       ///< 未还贡座位
        std::map<int, int> returnToTributer;       ///< 还贡座位 -> 收贡座位
    };
    InteractiveTributeState tributeState_{};

    RuleContext ruleContext() const;
    PlayResult dealAndStartPlaying(bool applyTribute);
    void beginPlayingFromDeal(int firstPlayerSeat);
    PlayResult finishTributePhaseAndStartPlay();
    void clearTributeState();
    void assignFinishRank(int playerIndex);
    bool checkTeamWin() const;
    void incrementStateVersion();
    std::vector<Card> cardsFromIds(const std::vector<CardId>& ids) const;
    int seatOf(PlayerId id) const;
};

}  // namespace guandan
