#include "game/bot_player.h"

#include <climits>
#include <vector>

namespace guandan {

namespace {

std::vector<Card> cardsFromHand(const GameState& state, int seatIndex,
                                const std::vector<CardId>& ids) {
    std::vector<Card> cards;
    cards.reserve(ids.size());
    for (CardId id : ids) {
        cards.push_back(state.getCardById(id));
    }
    return cards;
}

}  // namespace

int BotPlayer::leadScore(size_t cardCount, const CardPattern& pattern) {
    return static_cast<int>(cardCount) * 10000 + pattern.primaryRank * 10 +
           static_cast<int>(pattern.type);
}

int BotPlayer::beatScore(size_t cardCount, const CardPattern& pattern) {
    return static_cast<int>(cardCount) * 10000 + pattern.primaryRank * 10 +
           static_cast<int>(pattern.type);
}

std::optional<std::vector<CardId>> BotPlayer::choosePlay(
    const GameState& state,
    int seatIndex,
    const RuleContext& ctx,
    const CardAnalyzer& analyzer,
    const RuleEngine& rules
) const {
    if (seatIndex < 0 || seatIndex >= state.playerCount) return std::nullopt;
    if (state.players[seatIndex].hasFinished) return std::nullopt;

    const auto& handIds = state.players[seatIndex].hand.cards();
    const int n = static_cast<int>(handIds.size());
    if (n == 0) return std::nullopt;

    const bool isLead = !state.lastPattern.isValid;
    std::optional<std::vector<CardId>> best;
    int bestScore = INT_MAX;

    const int maxMask = 1 << n;
    for (int mask = 1; mask < maxMask; ++mask) {
        std::vector<CardId> subset;
        subset.reserve(n);
        for (int i = 0; i < n; ++i) {
            if (mask & (1 << i)) subset.push_back(handIds[i]);
        }

        const auto cards = cardsFromHand(state, seatIndex, subset);
        const CardPattern pattern = analyzer.analyze(cards, ctx);
        if (!pattern.isValid) continue;

        if (isLead) {
            const int score = leadScore(subset.size(), pattern);
            if (score < bestScore) {
                bestScore = score;
                best = subset;
            }
        } else if (rules.canBeat(pattern, state.lastPattern, ctx)) {
            const int score = beatScore(subset.size(), pattern);
            if (score < bestScore) {
                bestScore = score;
                best = subset;
            }
        }
    }

    return best;
}

}  // namespace guandan
