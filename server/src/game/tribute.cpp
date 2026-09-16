#include "game/tribute.h"

#include <algorithm>
#include <map>
#include <sstream>

namespace guandan {

namespace {

int rankOrderForTribute(const Card& card, const RuleContext& ctx) {
    if (isJoker(card.rank)) return rankValue(card.rank);
    if (ctx.isWildCard(card)) return rankValue(ctx.levelRank());
    return rankValue(card.rank);
}

}  // namespace

PreviousRoundInfo buildPreviousRoundInfo(const GameState& state, int winningTeam) {
    PreviousRoundInfo info;
    info.valid = true;
    info.winningTeam = winningTeam;
    for (int i = 0; i < state.playerCount; ++i) {
        info.finishRankBySeat[i] = state.players[i].finishRank;
    }
    return info;
}

int TributeManager::seatByFinishRank(const PreviousRoundInfo& previous, int rank, int playerCount) const {
    for (int seat = 0; seat < playerCount; ++seat) {
        if (previous.finishRankBySeat[seat] == rank) return seat;
    }
    return -1;
}

bool TributeManager::isValidTributeCard(const Card& card, const RuleContext& ctx) const {
    return !ctx.isWildCard(card);
}

bool TributeManager::isValidReturnCard(const Card& card, const RuleContext& ctx) const {
    if (ctx.isLevelCard(card) || isJoker(card.rank)) return false;
    return rankValue(card.rank) <= rankValue(Rank::R10);
}

CardId TributeManager::requiredTributeCard(
    const Hand& hand,
    const GameState& state,
    const RuleContext& ctx
) const {
    return pickBestTributeCard(hand, state, ctx);
}

CardId TributeManager::pickBestTributeCard(
    const Hand& hand,
    const GameState& state,
    const RuleContext& ctx
) const {
    CardId best = 0;
    int bestOrder = -1;
    for (CardId id : hand.cards()) {
        const Card& card = state.getCardById(id);
        if (!isValidTributeCard(card, ctx)) continue;
        const int order = rankOrderForTribute(card, ctx);
        if (order > bestOrder) {
            bestOrder = order;
            best = id;
        }
    }
    return best;
}

int TributeManager::minSingleEffectiveRank(
    const Hand& hand,
    const GameState& state,
    const RuleContext& ctx
) const {
    int bestOrder = 999;
    for (CardId id : hand.cards()) {
        bestOrder = std::min(
            bestOrder,
            singleEffectiveRank(state.getCardById(id), ctx));
    }
    return bestOrder == 999 ? 0 : bestOrder;
}

CardId TributeManager::pickSmallestCard(
    const Hand& hand,
    const GameState& state,
    const RuleContext& ctx
) const {
    const int minOrder = minSingleEffectiveRank(hand, state, ctx);
    if (minOrder == 0) return 0;

    CardId best = 0;
    for (CardId id : hand.cards()) {
        if (singleEffectiveRank(state.getCardById(id), ctx) != minOrder) continue;
        best = id;
        break;
    }
    return best;
}

CardId TributeManager::pickBestReturnCard(
    const Hand& hand,
    const GameState& state,
    const RuleContext& ctx
) const {
    CardId best = 0;
    int bestOrder = 999;
    for (CardId id : hand.cards()) {
        const Card& card = state.getCardById(id);
        if (!isValidReturnCard(card, ctx)) continue;
        const int order = rankValue(card.rank);
        if (order < bestOrder) {
            bestOrder = order;
            best = id;
        }
    }
    return best;
}

CardId TributeManager::pickReturnCard(
    const Hand& hand,
    const GameState& state,
    const RuleContext& ctx
) const {
    const CardId valid = pickBestReturnCard(hand, state, ctx);
    if (valid != 0) return valid;
    return pickSmallestCard(hand, state, ctx);
}

std::vector<CardId> TributeManager::validReturnCardIds(
    const Hand& hand,
    const GameState& state,
    const RuleContext& ctx
) const {
    std::vector<CardId> primary;
    for (CardId id : hand.cards()) {
        if (isValidReturnCard(state.getCardById(id), ctx)) {
            primary.push_back(id);
        }
    }
    if (!primary.empty()) return primary;

    const int minOrder = minSingleEffectiveRank(hand, state, ctx);
    if (minOrder == 0) return {};

    std::vector<CardId> fallback;
    for (CardId id : hand.cards()) {
        if (singleEffectiveRank(state.getCardById(id), ctx) == minOrder) {
            fallback.push_back(id);
        }
    }
    return fallback;
}

bool TributeManager::isValidTributeSubmission(
    int seat,
    CardId cardId,
    const GameState& state,
    const RuleContext& ctx
) const {
    if (seat < 0 || seat >= state.playerCount || cardId == 0) return false;
    const auto& hand = state.players[seat].hand;
    if (!hand.contains({cardId})) return false;
    return cardId == requiredTributeCard(hand, state, ctx);
}

bool TributeManager::isValidReturnSubmission(
    int seat,
    CardId cardId,
    const GameState& state,
    const RuleContext& ctx
) const {
    if (seat < 0 || seat >= state.playerCount || cardId == 0) return false;
    const auto& hand = state.players[seat].hand;
    if (!hand.contains({cardId})) return false;

    const auto validIds = validReturnCardIds(hand, state, ctx);
    return std::find(validIds.begin(), validIds.end(), cardId) != validIds.end();
}

int TributeManager::countBigJokersInSeat(const GameState& state, int seat) const {
    int count = 0;
    for (CardId id : state.players[seat].hand.cards()) {
        if (state.getCardById(id).rank == Rank::BIG_JOKER) ++count;
    }
    return count;
}

int TributeManager::compareTributeCards(
    CardId a,
    CardId b,
    const GameState& state,
    const RuleContext& ctx,
    int seatA,
    int seatB,
    int headSeat,
    int playerCount
) const {
    const Card& ca = state.getCardById(a);
    const Card& cb = state.getCardById(b);
    const int orderA = rankOrderForTribute(ca, ctx);
    const int orderB = rankOrderForTribute(cb, ctx);
    if (orderA != orderB) return orderA > orderB ? -1 : 1;

    const int distA = (seatA - headSeat + playerCount) % playerCount;
    const int distB = (seatB - headSeat + playerCount) % playerCount;
    if (distA != distB) return distA < distB ? -1 : 1;
    return 0;
}

bool TributeManager::checkAntiTribute4(
    const GameState& state,
    bool doubleDown,
    const std::vector<int>& tributerSeats
) const {
    if (doubleDown) {
        for (int seat : tributerSeats) {
            if (countBigJokersInSeat(state, seat) >= 2) return true;
        }
        int singleBig = 0;
        for (int seat : tributerSeats) {
            if (countBigJokersInSeat(state, seat) == 1) ++singleBig;
        }
        return singleBig >= 2;
    }

    int total = 0;
    for (int seat : tributerSeats) {
        total += countBigJokersInSeat(state, seat);
    }
    return total >= 2;
}

bool TributeManager::checkAntiTribute6(
    const GameState& state,
    bool tripleDown,
    const std::vector<int>& tributerSeats
) const {
    if (tripleDown) {
        for (int seat : tributerSeats) {
            if (countBigJokersInSeat(state, seat) >= 3) return true;
        }
        int total = 0;
        for (int seat : tributerSeats) {
            total += countBigJokersInSeat(state, seat);
        }
        return total >= 3;
    }

    for (int seat : tributerSeats) {
        if (countBigJokersInSeat(state, seat) >= 3) return true;
    }
    return false;
}

void TributeManager::transferCard(GameState& state, int fromSeat, int toSeat, CardId cardId) const {
    if (fromSeat < 0 || toSeat < 0 || cardId == 0) return;
    std::vector<CardId> one{cardId};
    if (!state.players[fromSeat].hand.contains(one)) return;
    state.players[fromSeat].hand.remove(one);
    state.players[toSeat].hand.add(cardId);
}

void TributeManager::applyReturnTransfer(
    GameState& state,
    int fromSeat,
    int toSeat,
    CardId cardId
) const {
    transferCard(state, fromSeat, toSeat, cardId);
}

TributePhasePlan TributeManager::planRound(
    const GameState& state,
    const GameRuleConfig& config,
    const PreviousRoundInfo& previous
) const {
    TributePhasePlan plan;
    const int n = state.playerCount;

    if (!config.enableTribute || !previous.valid) {
        plan.skipped = true;
        plan.headSeat = seatByFinishRank(previous, 1, n);
        if (plan.headSeat < 0) plan.headSeat = 0;
        plan.summary = "skip_no_previous_round";
        return plan;
    }

    plan.skipped = false;
    plan.headSeat = seatByFinishRank(previous, 1, n);
    const int secondSeat = seatByFinishRank(previous, 2, n);
    const int thirdSeat = seatByFinishRank(previous, 3, n);
    if (plan.headSeat < 0) {
        plan.skipped = true;
        plan.summary = "skip_no_head";
        return plan;
    }

    if (config.playersPerTeam >= 3) {
        const bool tripleDown =
            secondSeat >= 0 && thirdSeat >= 0 &&
            state.players[plan.headSeat].team == state.players[secondSeat].team &&
            state.players[plan.headSeat].team == state.players[thirdSeat].team;

        if (tripleDown) {
            for (int rank = 4; rank <= 6; ++rank) {
                const int seat = seatByFinishRank(previous, rank, n);
                if (seat >= 0) plan.tributerSeats.push_back(seat);
            }
            plan.recipientSeats = {plan.headSeat, secondSeat, thirdSeat};
            plan.summary = "triple_down";
        } else {
            const int sixthSeat = seatByFinishRank(previous, 6, n);
            if (sixthSeat >= 0) plan.tributerSeats.push_back(sixthSeat);
            plan.recipientSeats = {plan.headSeat};
            plan.summary = "single_down";
        }

        if (checkAntiTribute6(state, tripleDown, plan.tributerSeats)) {
            plan.antiTribute = true;
            plan.summary += "_anti";
            return plan;
        }
    } else {
        const bool doubleDown =
            secondSeat >= 0 &&
            state.players[plan.headSeat].team == state.players[secondSeat].team;

        if (doubleDown) {
            for (int rank = 3; rank <= 4; ++rank) {
                const int seat = seatByFinishRank(previous, rank, n);
                if (seat >= 0) plan.tributerSeats.push_back(seat);
            }
            plan.recipientSeats = {plan.headSeat, secondSeat};
            plan.summary = "double_down";
        } else {
            const int fourthSeat = seatByFinishRank(previous, 4, n);
            if (fourthSeat >= 0) plan.tributerSeats.push_back(fourthSeat);
            plan.recipientSeats = {plan.headSeat};
            plan.summary = "single_down";
        }

        if (checkAntiTribute4(state, doubleDown, plan.tributerSeats)) {
            plan.antiTribute = true;
            plan.summary += "_anti";
            return plan;
        }
    }

    return plan;
}

void TributeManager::assignTributes(
    GameState& state,
    const RuleContext& ctx,
    const TributePhasePlan& plan,
    const std::map<int, CardId>& submissions,
    TributeRoundResult& result
) const {
    result.tributes.clear();
    const int n = state.playerCount;

    struct PendingTribute {
        int fromSeat;
        CardId cardId;
    };
    std::vector<PendingTribute> pending;
    for (int seat : plan.tributerSeats) {
        const auto it = submissions.find(seat);
        if (it == submissions.end() || it->second == 0) continue;
        pending.push_back({seat, it->second});
    }

    if (pending.empty()) {
        result.summary = plan.summary + "_no_card";
        return;
    }

    std::sort(pending.begin(), pending.end(), [&](const PendingTribute& a, const PendingTribute& b) {
        return compareTributeCards(
            a.cardId, b.cardId, state, ctx, a.fromSeat, b.fromSeat, plan.headSeat, n) < 0;
    });

    const int assignCount = std::min(
        static_cast<int>(pending.size()),
        static_cast<int>(plan.recipientSeats.size()));
    for (int i = 0; i < assignCount; ++i) {
        const auto& tribute = pending[i];
        const int toSeat = plan.recipientSeats[i];
        transferCard(state, tribute.fromSeat, toSeat, tribute.cardId);
        result.tributes.push_back({tribute.fromSeat, toSeat, tribute.cardId});
    }
    result.summary = plan.summary;
}

int TributeManager::computeFirstPlayerSeat(const TributeRoundResult& result, int headSeat) const {
    for (const auto& tr : result.tributes) {
        if (tr.toSeat == headSeat) {
            return tr.fromSeat;
        }
    }
    return headSeat;
}

TributeRoundResult TributeManager::resolveRound(
    GameState& state,
    const GameRuleConfig& config,
    const PreviousRoundInfo& previous,
    const RuleContext& ctx
) const {
    TributeRoundResult result;
    const auto plan = planRound(state, config, previous);

    if (plan.skipped) {
        result.skipped = true;
        result.firstPlayerSeat = plan.headSeat;
        result.summary = plan.summary;
        return result;
    }

    if (plan.antiTribute) {
        result.antiTribute = true;
        result.firstPlayerSeat = plan.headSeat;
        result.summary = plan.summary;
        return result;
    }

    std::map<int, CardId> submissions;
    for (int seat : plan.tributerSeats) {
        const CardId cardId = requiredTributeCard(state.players[seat].hand, state, ctx);
        if (cardId != 0) submissions[seat] = cardId;
    }

    assignTributes(state, ctx, plan, submissions, result);
    if (result.tributes.empty()) {
        result.firstPlayerSeat = plan.headSeat;
        return result;
    }

    for (const auto& tr : result.tributes) {
        const CardId returnId = pickReturnCard(state.players[tr.toSeat].hand, state, ctx);
        if (returnId == 0) continue;
        transferCard(state, tr.toSeat, tr.fromSeat, returnId);
        result.returns.push_back({tr.toSeat, tr.fromSeat, returnId});
    }

    result.headTributerSeat = -1;
    for (const auto& tr : result.tributes) {
        if (tr.toSeat == plan.headSeat) {
            result.headTributerSeat = tr.fromSeat;
            break;
        }
    }
    result.firstPlayerSeat = computeFirstPlayerSeat(result, plan.headSeat);
    return result;
}

}  // namespace guandan
