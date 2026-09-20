#pragma once

#include "game/card.h"
#include "game/game_state.h"
#include "game/types.h"
#include <array>
#include <map>
#include <string>
#include <vector>

namespace guandan {

struct PreviousRoundInfo {
    bool valid = false;
    int winningTeam = -1;
    std::array<int, kPlayerCount> finishRankBySeat{};
};

struct TributeTransfer {
    int fromSeat = -1;
    int toSeat = -1;
    CardId cardId = 0;
};

struct TributeRoundResult {
    bool skipped = false;          // 首局或未开启进贡
    bool antiTribute = false;      // 抗贡
    std::vector<TributeTransfer> tributes;
    std::vector<TributeTransfer> returns;
    int firstPlayerSeat = 0;
    int headTributerSeat = -1;     // 给头游进贡的玩家（首出）
    std::string summary;
};

/// 进贡阶段计划（尚未转移牌）。
struct TributePhasePlan {
    bool skipped = true;
    bool antiTribute = false;
    std::vector<int> tributerSeats;
    std::vector<int> recipientSeats;
    int headSeat = 0;
    std::string summary;
};

/// 4/6 人掼蛋进贡、还贡、抗贡规则。
class TributeManager {
public:
    TributePhasePlan planRound(
        const GameState& state,
        const GameRuleConfig& config,
        const PreviousRoundInfo& previous
    ) const;

    TributeRoundResult resolveRound(
        GameState& state,
        const GameRuleConfig& config,
        const PreviousRoundInfo& previous,
        const RuleContext& ctx
    ) const;

    bool isValidTributeCard(const Card& card, const RuleContext& ctx) const;
    bool isValidReturnCard(const Card& card, const RuleContext& ctx) const;

    /// 进贡牌：手牌中最大非逢人配（非红桃级牌）。
    CardId requiredTributeCard(
        const Hand& hand,
        const GameState& state,
        const RuleContext& ctx
    ) const;

    bool isValidTributeSubmission(
        int seat,
        CardId cardId,
        const GameState& state,
        const RuleContext& ctx
    ) const;

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

    void assignTributes(
        GameState& state,
        const RuleContext& ctx,
        const TributePhasePlan& plan,
        const std::map<int, CardId>& submissions,
        TributeRoundResult& result
    ) const;

    void applyReturnTransfer(
        GameState& state,
        int fromSeat,
        int toSeat,
        CardId cardId
    ) const;

    int computeFirstPlayerSeat(
        const TributeRoundResult& result,
        int headSeat,
        const GameState& state,
        const RuleContext& ctx,
        const PreviousRoundInfo& previous,
        int playerCount
    ) const;

    CardId pickBestTributeCard(
        const Hand& hand,
        const GameState& state,
        const RuleContext& ctx
    ) const;

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

PreviousRoundInfo buildPreviousRoundInfo(const GameState& state, int winningTeam);

}  // namespace guandan
