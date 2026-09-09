#include "game/rule_engine.h"

namespace guandan {

bool RuleEngine::isBombType(CardType type) const {
    return type == CardType::BOMB || type == CardType::JOKER_BOMB;
}

bool RuleEngine::sameTypeBeat(const CardPattern& cur, const CardPattern& prev) const {
    if (cur.type != prev.type) return false;

    switch (cur.type) {
        case CardType::SINGLE:
        case CardType::PAIR:
        case CardType::TRIPLE:
        case CardType::STRAIGHT:
        case CardType::THREE_PAIRS:
        case CardType::TWO_TRIPLES:
        case CardType::STRAIGHT_FLUSH:
            if (cur.length != prev.length) return false;
            return cur.primaryRank > prev.primaryRank;

        case CardType::TRIPLE_WITH_PAIR:
            if (cur.primaryRank != prev.primaryRank) {
                return cur.primaryRank > prev.primaryRank;
            }
            return cur.pairRank > prev.pairRank;

        case CardType::BOMB:
            if (cur.length != prev.length) return cur.length > prev.length;
            return cur.primaryRank > prev.primaryRank;

        case CardType::JOKER_BOMB:
            return cur.length > prev.length;

        default:
            return false;
    }
}

bool RuleEngine::canBeat(
    const CardPattern& current,
    const CardPattern& previous,
    const RuleContext& /*context*/
) const {
    if (!current.isValid) return false;
    if (!previous.isValid) return true;  // 新一轮首出

    if (current.type == CardType::JOKER_BOMB) {
        if (previous.type == CardType::JOKER_BOMB) {
            return current.length > previous.length;
        }
        return true;
    }

    if (current.type == CardType::STRAIGHT_FLUSH) {
        if (previous.type == CardType::JOKER_BOMB) return false;
        if (previous.type == CardType::BOMB) {
            return previous.length < 6;
        }
        if (previous.type == CardType::STRAIGHT_FLUSH) {
            return sameTypeBeat(current, previous);
        }
        return true;
    }

    if (current.type == CardType::BOMB) {
        if (previous.type == CardType::JOKER_BOMB) return false;
        if (previous.type == CardType::STRAIGHT_FLUSH) {
            return current.length >= 6;
        }
        if (previous.type == CardType::BOMB) {
            if (current.length != previous.length) return current.length > previous.length;
            return current.primaryRank > previous.primaryRank;
        }
        return true;
    }

    if (previous.type == CardType::JOKER_BOMB ||
        previous.type == CardType::STRAIGHT_FLUSH ||
        previous.type == CardType::BOMB) {
        return false;
    }

    return sameTypeBeat(current, previous);
}

}  // namespace guandan
