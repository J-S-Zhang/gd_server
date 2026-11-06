#pragma once

#include "game/types.h"
#include <string>
#include <vector>

namespace guandan {

/// 单张牌
struct Card {
    CardId id = 0;              ///< 全局唯一 id
    Suit suit = Suit::SPADE;    ///< 花色
    Rank rank = Rank::R2;       ///< 点数

    bool operator==(const Card& other) const {
        return id == other.id;
    }

    /// 可读字符串，如 10H、SJ
    std::string toString() const {
        if (suit == Suit::JOKER) {
            return rankToString(rank);
        }
        return rankToString(rank) + suitToString(suit);
    }
};

/// 规则与超时等引擎配置
struct GameRuleConfig {
    int playerCount = kPlayerCount;           ///< 玩家数
    int deckCount = kDeckCount;               ///< 副数
    int teamCount = kTeamCount;               ///< 队数
    int playersPerTeam = kPlayersPerTeam;     ///< 每队人数
    bool enableTribute = false;               ///< 进贡
    bool enableReturnTribute = false;         ///< 还贡
    bool enableAntiTribute = false;           ///< 抗贡
    bool enableWildCard = true;               ///< 逢人配（红桃级牌）
    int turnTimeoutSeconds = 30;              ///< 出牌超时秒数
    std::string ruleVersion = "six_player_guandan_v1";  ///< 规则版本标识
};

constexpr int kLevelCardPatternRank = 15;   ///< 级牌在指定牌型中大于 A(14)
constexpr int kSmallJokerPatternRank = 16;  ///< 小王比较秩
constexpr int kBigJokerPatternRank = 17;    ///< 大王比较秩

/// 当前局级牌与逢人配上下文
struct RuleContext {
    int currentLevel = 2;  ///< 当前级牌 2-14 (2 to A)
    bool enableWildCard = true;  ///< 是否启用逢人配

    /// 当前级牌 Rank
    Rank levelRank() const {
        return static_cast<Rank>(currentLevel);
    }

    /// 级牌原始 rank 值
    int rawLevelRankValue() const {
        return rankValue(levelRank());
    }

    /// 是否为逢人配（红桃级牌）
    bool isWildCard(const Card& card) const {
        if (!enableWildCard) return false;
        return card.suit == Suit::HEART && card.rank == levelRank();
    }

    /// 是否为级牌（非王且点数等于当前级）
    bool isLevelCard(const Card& card) const {
        return !isJoker(card.rank) && card.rank == levelRank();
    }
};

/// 单牌 / 对子 / 三不带 / 三带二(仅三张部分) / 纯级牌炸弹：级牌按级牌比较。
inline int patternRankForLevelCard(int rawRank, const RuleContext& ctx) {
    if (rawRank == ctx.rawLevelRankValue()) return kLevelCardPatternRank;
    return rawRank;
}

/// 单张在规则下的有效比较秩
inline int singleEffectiveRank(const Card& card, const RuleContext& ctx) {
    if (card.rank == Rank::SMALL_JOKER) return kSmallJokerPatternRank;
    if (card.rank == Rank::BIG_JOKER) return kBigJokerPatternRank;
    if (ctx.isLevelCard(card)) return kLevelCardPatternRank;
    return rankValue(card.rank);
}

/// 构造一张牌
Card makeCard(CardId id, Suit suit, Rank rank);

/// 创建完整牌堆（多副）
std::vector<Card> createFullDeck(int deckCount = kDeckCount);

}  // namespace guandan
