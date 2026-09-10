#pragma once

#include "game/card.h"
#include "game/card_pattern.h"
#include <map>
#include <vector>

namespace guandan {

class CardAnalyzer {
public:
    CardPattern analyze(
        const std::vector<Card>& cards,
        const RuleContext& context
    ) const;

private:
    struct RankGroup {
        int rank = 0;
        int count = 0;
        std::vector<const Card*> cards;
    };

    std::vector<RankGroup> groupByRank(
        const std::vector<Card>& cards,
        const RuleContext& context
    ) const;

    CardPattern analyzeJokerBomb(const std::vector<Card>& cards) const;
    CardPattern analyzeBomb(
        const std::vector<Card>& cards,
        const std::vector<RankGroup>& groups,
        size_t total,
        const RuleContext& ctx
    ) const;
    CardPattern analyzeSingle(const std::vector<Card>& cards, const RuleContext& ctx) const;
    CardPattern analyzePair(
        const std::vector<RankGroup>& groups,
        int wildCount,
        const RuleContext& ctx
    ) const;
    CardPattern analyzeTriple(
        const std::vector<RankGroup>& groups,
        int wildCount,
        const RuleContext& ctx
    ) const;
    CardPattern analyzeTripleWithPair(
        const std::vector<RankGroup>& groups,
        int wildCount,
        const RuleContext& ctx
    ) const;
    CardPattern analyzeStraight(const std::vector<RankGroup>& groups, int wildCount, size_t total) const;
    CardPattern analyzeThreePairs(const std::vector<RankGroup>& groups, int wildCount, size_t total) const;
    CardPattern analyzeTwoTriples(const std::vector<RankGroup>& groups, int wildCount, size_t total) const;
    CardPattern analyzeStraightFlush(const std::vector<Card>& cards, const RuleContext& ctx) const;

    int effectiveRank(const Card& card, const RuleContext& ctx) const;
    bool isConsecutive(const std::vector<int>& ranks) const;
};

}  // namespace guandan
