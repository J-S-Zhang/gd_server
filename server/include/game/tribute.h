#pragma once

#include "game/card.h"
#include "game/game_state.h"
#include "game/types.h"
#include <array>
#include <map>
#include <string>
#include <vector>

namespace guandan {

/// 上一局结果摘要（用于本局进贡）
struct PreviousRoundInfo {
    bool valid = false;                              ///< 是否有有效上一局
    int winningTeam = -1;                            ///< 胜队
    std::array<int, kPlayerCount> finishRankBySeat{};  ///< 各座位游次
};

/// 单张进贡或还贡转移
struct TributeTransfer {
    int fromSeat = -1;  ///< 出牌座位
    int toSeat = -1;    ///< 收牌座位
    CardId cardId = 0;  ///< 牌 id
};

/// 一局进贡/还贡解析结果
struct TributeRoundResult {
    bool skipped = false;          ///< 首局或未开启进贡
    bool antiTribute = false;      ///< 抗贡
    std::vector<TributeTransfer> tributes;  ///< 进贡转移
    std::vector<TributeTransfer> returns;   ///< 还贡转移
    int firstPlayerSeat = 0;     ///< 本局首出座位
    int headTributerSeat = -1;   ///< 给头游进贡的玩家（首出相关）
    std::string summary;           ///< 文字摘要
};

/// 进贡阶段计划（尚未转移牌）。
struct TributePhasePlan {
    bool skipped = true;              ///< 是否跳过进贡
    bool antiTribute = false;         ///< 是否抗贡
    std::vector<int> tributerSeats;   ///< 需进贡座位
    std::vector<int> recipientSeats;  ///< 收贡座位
    int headSeat = 0;                 ///< 头游座位
    std::string summary;              ///< 摘要
};

/// 4/6 人掼蛋进贡、还贡、抗贡规则。
class TributeManager {
public:
    /// 生成本局进贡计划（不挪牌）
    TributePhasePlan planRound(
        const GameState& state,
        const GameRuleConfig& config,
        const PreviousRoundInfo& previous
    ) const;

    /// 一次性解析并执行进贡还贡（非交互流程）
    TributeRoundResult resolveRound(
        GameState& state,
        const GameRuleConfig& config,
        const PreviousRoundInfo& previous,
        const RuleContext& ctx
    ) const;

    /// 是否可作为进贡牌
    bool isValidTributeCard(const Card& card, const RuleContext& ctx) const;
    /// 是否可作为还贡牌
    bool isValidReturnCard(const Card& card, const RuleContext& ctx) const;

    /// 进贡牌：手牌中最大非逢人配（非红桃级牌）。
    CardId requiredTributeCard(
        const Hand& hand,
        const GameState& state,
        const RuleContext& ctx
    ) const;

    /// 校验某座位提交的进贡牌
    bool isValidTributeSubmission(
        int seat,
        CardId cardId,
        const GameState& state,
        const RuleContext& ctx
    ) const;

    /// 校验某座位提交的还贡牌
    bool isValidReturnSubmission(
        int seat,
        CardId cardId,
        const GameState& state,
        const RuleContext& ctx
    ) const;

    /// 还贡推荐：优先 ≤10 非级牌中最小；否则按单牌大小还最小。
    CardId pickReturnCard(
        const Hand& hand,
        const GameState& state,
        const RuleContext& ctx
    ) const;

    /// 可还贡牌：优先所有 ≤10 非级牌；若无则所有单牌最小的牌（可多张自选）。
    std::vector<CardId> validReturnCardIds(
        const Hand& hand,
        const GameState& state,
        const RuleContext& ctx
    ) const;

    /// 根据提交执行进贡转移并写入 result
    void assignTributes(
        GameState& state,
        const RuleContext& ctx,
        const TributePhasePlan& plan,
        const std::map<int, CardId>& submissions,
        TributeRoundResult& result
    ) const;

    /// 执行单次还贡转移
    void applyReturnTransfer(
        GameState& state,
        int fromSeat,
        int toSeat,
        CardId cardId
    ) const;

    /// 计算本局首出座位
    int computeFirstPlayerSeat(
        const TributeRoundResult& result,
        int headSeat,
        const GameState& state,
        const RuleContext& ctx,
        const PreviousRoundInfo& previous,
        int playerCount
    ) const;

    /// 机器人：选进贡牌
    CardId pickBestTributeCard(
        const Hand& hand,
        const GameState& state,
        const RuleContext& ctx
    ) const;

    /// 机器人：选还贡牌
    CardId pickBestReturnCard(
        const Hand& hand,
        const GameState& state,
        const RuleContext& ctx
    ) const;

private:
    int seatByFinishRank(const PreviousRoundInfo& previous, int rank, int playerCount) const;
    int compareTributeCards(
        CardId a,
        CardId b,
        const GameState& state,
        const RuleContext& ctx,
        int seatA,
        int seatB,
        int headSeat,
        int playerCount
    ) const;

    bool checkAntiTribute4(
        const GameState& state,
        bool doubleDown,
        const std::vector<int>& tributerSeats
    ) const;

    bool checkAntiTribute6(
        const GameState& state,
        bool tripleDown,
        const std::vector<int>& tributerSeats
    ) const;

    int countBigJokersInSeat(const GameState& state, int seat) const;
    CardId pickSmallestCard(
        const Hand& hand,
        const GameState& state,
        const RuleContext& ctx
    ) const;
    int minSingleEffectiveRank(
        const Hand& hand,
        const GameState& state,
        const RuleContext& ctx
    ) const;
    void transferCard(GameState& state, int fromSeat, int toSeat, CardId cardId) const;
};

/// 从终局状态构建 PreviousRoundInfo
PreviousRoundInfo buildPreviousRoundInfo(const GameState& state, int winningTeam);

}  // namespace guandan
