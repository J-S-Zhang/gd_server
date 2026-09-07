#include "game/card_analyzer.h"
#include <algorithm>
#include <set>

namespace guandan {

int CardAnalyzer::effectiveRank(const Card& card, const RuleContext& ctx) const {
    if (isJoker(card.rank)) return rankValue(card.rank);
    if (ctx.isWildCard(card)) return rankValue(ctx.levelRank());
    return rankValue(card.rank);
}

std::vector<CardAnalyzer::RankGroup> CardAnalyzer::groupByRank(
    const std::vector<Card>& cards,
    const RuleContext& context
) const {
    std::map<int, RankGroup> groups;
    int wildCount = 0;

    for (const auto& card : cards) {
        if (context.isWildCard(card)) {
            ++wildCount;
            continue;
        }
        if (isJoker(card.rank)) {
            int r = rankValue(card.rank);
            groups[r].rank = r;
            groups[r].count++;
            groups[r].cards.push_back(&card);
            continue;
        }
        int r = rankValue(card.rank);
        groups[r].rank = r;
        groups[r].count++;
        groups[r].cards.push_back(&card);
    }

    std::vector<RankGroup> result;
    for (auto& [_, g] : groups) {
        result.push_back(g);
    }
    std::sort(result.begin(), result.end(),
              [](const RankGroup& a, const RankGroup& b) { return a.rank < b.rank; });

    if (wildCount > 0) {
        RankGroup wild;
        wild.rank = -1;
        wild.count = wildCount;
        result.insert(result.begin(), wild);
    }
    return result;
}

bool CardAnalyzer::isConsecutive(const std::vector<int>& ranks) const {
    if (ranks.size() < 2) return true;
    for (size_t i = 1; i < ranks.size(); ++i) {
        if (ranks[i] != ranks[i - 1] + 1) return false;
    }
    return true;
}

CardPattern CardAnalyzer::analyzeJokerBomb(const std::vector<Card>& cards) const {
    int smallCount = 0, bigCount = 0;
    for (const auto& c : cards) {
        if (c.rank == Rank::SMALL_JOKER) ++smallCount;
        else if (c.rank == Rank::BIG_JOKER) ++bigCount;
        else return CardPattern::invalid();
    }
    if (smallCount >= 2 && bigCount >= 2) {
        CardPattern p;
        p.type = CardType::JOKER_BOMB;
        p.length = static_cast<int>(cards.size());
        p.primaryRank = rankValue(Rank::BIG_JOKER);
        p.isValid = true;
        return p;
    }
    return CardPattern::invalid();
}

CardPattern CardAnalyzer::analyzeBomb(
    const std::vector<RankGroup>& groups, size_t total
) const {
    if (total < 4) return CardPattern::invalid();

    int wildCount = 0;
    std::vector<RankGroup> normal;
    for (const auto& g : groups) {
        if (g.rank == -1) wildCount = g.count;
        else normal.push_back(g);
    }

    if (normal.size() > 1) return CardPattern::invalid();

    int rank = normal.empty() ? 2 : normal[0].rank;
    int count = (normal.empty() ? 0 : normal[0].count) + wildCount;

    if (count >= 4 && count == static_cast<int>(total)) {
        CardPattern p;
        p.type = CardType::BOMB;
        p.primaryRank = rank;
        p.length = count;
        p.isValid = true;
        return p;
    }
    return CardPattern::invalid();
}

CardPattern CardAnalyzer::analyzeSingle(
    const std::vector<Card>& cards, const RuleContext& ctx
) const {
    if (cards.size() != 1) return CardPattern::invalid();
    CardPattern p;
    p.type = CardType::SINGLE;
    p.primaryRank = effectiveRank(cards[0], ctx);
    p.length = 1;
    p.isValid = true;
    return p;
}

CardPattern CardAnalyzer::analyzePair(
    const std::vector<RankGroup>& groups, int wildCount
) const {
    if (groups.size() == 0) return CardPattern::invalid();
    int totalWild = wildCount;
    for (const auto& g : groups) {
        if (g.rank == -1) totalWild = g.count;
    }

    std::vector<RankGroup> normal;
    for (const auto& g : groups) {
        if (g.rank != -1) normal.push_back(g);
    }

    if (normal.size() > 1) return CardPattern::invalid();
    if (normal.empty() && totalWild >= 2) {
        CardPattern p;
        p.type = CardType::PAIR;
        p.primaryRank = 2;
        p.length = 2;
        p.isValid = true;
        return p;
    }
    if (normal.size() == 1 && normal[0].count + totalWild == 2) {
        CardPattern p;
        p.type = CardType::PAIR;
        p.primaryRank = normal[0].rank;
        p.length = 2;
        p.isValid = true;
        return p;
    }
    return CardPattern::invalid();
}

CardPattern CardAnalyzer::analyzeTriple(
    const std::vector<RankGroup>& groups, int wildCount
) const {
    int totalWild = wildCount;
    std::vector<RankGroup> normal;
    for (const auto& g : groups) {
        if (g.rank == -1) totalWild = g.count;
        else normal.push_back(g);
    }
    if (normal.size() > 1) return CardPattern::invalid();
    if (normal.empty() && totalWild >= 3) {
        CardPattern p;
        p.type = CardType::TRIPLE;
        p.primaryRank = 2;
        p.length = 3;
        p.isValid = true;
        return p;
    }
    if (normal.size() == 1 && normal[0].count + totalWild == 3) {
        CardPattern p;
        p.type = CardType::TRIPLE;
        p.primaryRank = normal[0].rank;
        p.length = 3;
        p.isValid = true;
        return p;
    }
    return CardPattern::invalid();
}

CardPattern CardAnalyzer::analyzeTripleWithPair(
    const std::vector<RankGroup>& groups, int wildCount
) const {
    int totalWild = 0;
    std::vector<RankGroup> normal;
    for (const auto& g : groups) {
        if (g.rank == -1) totalWild = g.count;
        else normal.push_back(g);
    }

    if (normal.size() > 2) return CardPattern::invalid();

    // Try each normal group as triple, rest as pair
    for (size_t i = 0; i < normal.size(); ++i) {
        int tripleRank = normal[i].rank;
        int tripleNeed = std::max(0, 3 - normal[i].count);
        if (tripleNeed > totalWild) continue;
        int wildLeft = totalWild - tripleNeed;

        for (size_t j = 0; j < normal.size(); ++j) {
            if (i == j) continue;
            int pairNeed = std::max(0, 2 - normal[j].count);
            if (pairNeed > wildLeft) continue;
            if (normal[j].count + pairNeed == 2) {
                CardPattern p;
                p.type = CardType::TRIPLE_WITH_PAIR;
                p.primaryRank = tripleRank;
                p.pairRank = normal[j].rank;
                p.length = 5;
                p.isValid = true;
                return p;
            }
        }
    }

    // All wild + one rank groups
    if (normal.size() == 1 && normal[0].count + totalWild == 5) {
        // Could be triple(3) + pair(2) from same rank with wilds - invalid for 三带二
        if (normal[0].count >= 3) {
            CardPattern p;
            p.type = CardType::TRIPLE_WITH_PAIR;
            p.primaryRank = normal[0].rank;
            p.pairRank = normal[0].rank;
            p.length = 5;
            p.isValid = true;
            return p;
        }
    }

    if (normal.size() == 2) {
        // Sort by count descending, try triple + pair
        auto a = normal[0], b = normal[1];
        if (a.count > b.count) std::swap(a, b);
        int tripleNeed = std::max(0, 3 - b.count);
        int pairNeed = std::max(0, 2 - a.count);
        if (tripleNeed + pairNeed <= totalWild) {
            CardPattern p;
            p.type = CardType::TRIPLE_WITH_PAIR;
            p.primaryRank = b.rank;
            p.pairRank = a.rank;
            p.length = 5;
            p.isValid = true;
            return p;
        }
    }

    return CardPattern::invalid();
}

CardPattern CardAnalyzer::analyzeStraight(
    const std::vector<RankGroup>& groups, int wildCount, size_t total
) const {
    if (total < 5) return CardPattern::invalid();

    int totalWild = 0;
    std::map<int, int> rankCount;
    for (const auto& g : groups) {
        if (g.rank == -1) totalWild = g.count;
        else rankCount[g.rank] = g.count;
    }

    // No 2 or jokers in straights
    for (const auto& [r, _] : rankCount) {
        if (r >= rankValue(Rank::R2) && r <= rankValue(Rank::A)) continue;
        if (r == rankValue(Rank::R2)) return CardPattern::invalid();
        if (r >= rankValue(Rank::SMALL_JOKER)) return CardPattern::invalid();
    }

    int len = static_cast<int>(total);
    for (int start = 3; start <= rankValue(Rank::A) - len + 1; ++start) {
        int wildUsed = 0;
        bool ok = true;
        for (int i = 0; i < len; ++i) {
            int r = start + i;
            if (r > rankValue(Rank::A)) { ok = false; break; }
            if (r == rankValue(Rank::R2)) { ok = false; break; }
            auto it = rankCount.find(r);
            if (it == rankCount.end()) {
                wildUsed++;
            } else if (it->second > 1) {
                ok = false; break;
            }
        }
        if (ok && wildUsed <= totalWild) {
            CardPattern p;
            p.type = CardType::STRAIGHT;
            p.primaryRank = start + len - 1;
            p.length = len;
            p.isValid = true;
            return p;
        }
    }
    return CardPattern::invalid();
}

CardPattern CardAnalyzer::analyzeThreePairs(
    const std::vector<RankGroup>& groups, int wildCount, size_t total
) const {
    int pairCount = static_cast<int>(total) / 2;
    if (pairCount < 3 || static_cast<size_t>(pairCount * 2) != total) {
        return CardPattern::invalid();
    }

    int totalWild = 0;
    std::map<int, int> rankCount;
    for (const auto& g : groups) {
        if (g.rank == -1) totalWild = g.count;
        else rankCount[g.rank] = g.count;
    }

    for (const auto& [r, _] : rankCount) {
        if (r == rankValue(Rank::R2) || r >= rankValue(Rank::SMALL_JOKER)) {
            return CardPattern::invalid();
        }
    }

    for (int start = 3; start <= rankValue(Rank::A) - pairCount + 1; ++start) {
        int wildUsed = 0;
        bool ok = true;
        for (int i = 0; i < pairCount; ++i) {
            int r = start + i;
            auto it = rankCount.find(r);
            int have = (it == rankCount.end()) ? 0 : it->second;
            if (have > 2) { ok = false; break; }
            wildUsed += std::max(0, 2 - have);
        }
        if (ok && wildUsed <= totalWild) {
            CardPattern p;
            p.type = CardType::THREE_PAIRS;
            p.primaryRank = start + pairCount - 1;
            p.length = pairCount;
            p.isValid = true;
            return p;
        }
    }
    return CardPattern::invalid();
}

CardPattern CardAnalyzer::analyzeTwoTriples(
    const std::vector<RankGroup>& groups, int wildCount, size_t total
) const {
    if (total != 6) return CardPattern::invalid();

    int totalWild = 0;
    std::map<int, int> rankCount;
    for (const auto& g : groups) {
        if (g.rank == -1) totalWild = g.count;
        else rankCount[g.rank] = g.count;
    }

    for (int start = 3; start <= rankValue(Rank::A) - 1; ++start) {
        int wildUsed = 0;
        bool ok = true;
        for (int i = 0; i < 2; ++i) {
            int r = start + i;
            if (r == rankValue(Rank::R2)) { ok = false; break; }
            auto it = rankCount.find(r);
            int have = (it == rankCount.end()) ? 0 : it->second;
            if (have > 3) { ok = false; break; }
            wildUsed += std::max(0, 3 - have);
        }
        if (ok && wildUsed <= totalWild) {
            CardPattern p;
            p.type = CardType::TWO_TRIPLES;
            p.primaryRank = start + 1;
            p.length = 2;
            p.isValid = true;
            return p;
        }
    }
    return CardPattern::invalid();
}

CardPattern CardAnalyzer::analyzeStraightFlush(
    const std::vector<Card>& cards, const RuleContext& ctx
) const {
    if (cards.size() < 5) return CardPattern::invalid();

    std::vector<Card> nonWild;
    int wildCount = 0;
    for (const auto& c : cards) {
        if (ctx.isWildCard(c)) wildCount++;
        else if (isJoker(c.rank)) return CardPattern::invalid();
        else nonWild.push_back(c);
    }

    if (nonWild.empty()) return CardPattern::invalid();

    Suit suit = nonWild[0].suit;
    for (const auto& c : nonWild) {
        if (c.suit != suit) return CardPattern::invalid();
    }

    auto groups = groupByRank(cards, ctx);
    auto straight = analyzeStraight(groups, wildCount, cards.size());
    if (straight.isValid) {
        straight.type = CardType::STRAIGHT_FLUSH;
        return straight;
    }
    return CardPattern::invalid();
}

CardPattern CardAnalyzer::analyze(
    const std::vector<Card>& cards,
    const RuleContext& context
) const {
    if (cards.empty()) return CardPattern::invalid();

    auto jokerBomb = analyzeJokerBomb(cards);
    if (jokerBomb.isValid) return jokerBomb;

    auto groups = groupByRank(cards, context);
    int wildCount = 0;
    for (const auto& g : groups) {
        if (g.rank == -1) wildCount = g.count;
    }

    auto bomb = analyzeBomb(groups, cards.size());
    if (bomb.isValid) return bomb;

    if (cards.size() == 1) return analyzeSingle(cards, context);
    if (cards.size() == 2) return analyzePair(groups, wildCount);
    if (cards.size() == 3) return analyzeTriple(groups, wildCount);
    if (cards.size() == 5) {
        auto twp = analyzeTripleWithPair(groups, wildCount);
        if (twp.isValid) return twp;
    }

    auto sf = analyzeStraightFlush(cards, context);
    if (sf.isValid) return sf;

    auto straight = analyzeStraight(groups, wildCount, cards.size());
    if (straight.isValid) return straight;

    if (cards.size() >= 6 && cards.size() % 2 == 0) {
        auto tp = analyzeThreePairs(groups, wildCount, cards.size());
        if (tp.isValid) return tp;
    }

    if (cards.size() == 6) {
        auto tt = analyzeTwoTriples(groups, wildCount, cards.size());
        if (tt.isValid) return tt;
    }

    return CardPattern::invalid();
}

}  // namespace guandan
