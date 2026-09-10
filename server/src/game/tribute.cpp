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

    // 牌点相同：从头游起顺时针，距离更近者优先（对应更大进贡牌）
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

TributeRoundResult TributeManager::resolveRound(
    GameState& state,
    const GameRuleConfig& config,
    const PreviousRoundInfo& previous,
    const RuleContext& ctx
) const {
    TributeRoundResult result;
    const int n = state.playerCount;

    if (!config.enableTribute || !previous.valid) {
        result.skipped = true;
        result.firstPlayerSeat = seatByFinishRank(previous, 1, n);
        if (result.firstPlayerSeat < 0) result.firstPlayerSeat = 0;
        result.summary = "skip_no_previous_round";
        return result;
    }

    const int headSeat = seatByFinishRank(previous, 1, n);
    const int secondSeat = seatByFinishRank(previous, 2, n);
    const int thirdSeat = seatByFinishRank(previous, 3, n);
    if (headSeat < 0) {
        result.skipped = true;
        result.firstPlayerSeat = 0;
        return result;
    }

    std::vector<int> tributerSeats;
    std::vector<int> recipientSeats;

    if (config.playersPerTeam >= 3) {
        // 6 人局
        const bool tripleDown =
            secondSeat >= 0 && thirdSeat >= 0 &&
            state.players[headSeat].team == state.players[secondSeat].team &&
            state.players[headSeat].team == state.players[thirdSeat].team;

        if (tripleDown) {
            for (int rank = 4; rank <= 6; ++rank) {
                const int seat = seatByFinishRank(previous, rank, n);
                if (seat >= 0) tributerSeats.push_back(seat);
            }
            recipientSeats = {headSeat, secondSeat, thirdSeat};
            result.summary = "triple_down";
        } else {
            const int sixthSeat = seatByFinishRank(previous, 6, n);
            if (sixthSeat >= 0) tributerSeats.push_back(sixthSeat);
            recipientSeats = {headSeat};
            result.summary = "single_down";
        }

        if (checkAntiTribute6(state, tripleDown, tributerSeats)) {
            result.antiTribute = true;
            result.firstPlayerSeat = headSeat;
            result.summary += "_anti";
            return result;
        }
    } else {
        // 4 人局
        const bool doubleDown =
            secondSeat >= 0 &&
            state.players[headSeat].team == state.players[secondSeat].team;

        if (doubleDown) {
            for (int rank = 3; rank <= 4; ++rank) {
                const int seat = seatByFinishRank(previous, rank, n);
                if (seat >= 0) tributerSeats.push_back(seat);
            }
            recipientSeats = {headSeat, secondSeat};
            result.summary = "double_down";
        } else {
            const int fourthSeat = seatByFinishRank(previous, 4, n);
            if (fourthSeat >= 0) tributerSeats.push_back(fourthSeat);
            recipientSeats = {headSeat};
            result.summary = "single_down";
        }

        if (checkAntiTribute4(state, doubleDown, tributerSeats)) {
            result.antiTribute = true;
            result.firstPlayerSeat = headSeat;
            result.summary += "_anti";
            return result;
        }
    }

    struct PendingTribute {
        int fromSeat;
        CardId cardId;
    };
    std::vector<PendingTribute> pending;
    for (int seat : tributerSeats) {
        CardId cardId = pickBestTributeCard(state.players[seat].hand, state, ctx);
        if (cardId == 0) continue;
        pending.push_back({seat, cardId});
    }

    if (pending.empty()) {
        result.firstPlayerSeat = headSeat;
        result.summary += "_no_card";
        return result;
    }

    std::sort(pending.begin(), pending.end(), [&](const PendingTribute& a, const PendingTribute& b) {
        return compareTributeCards(
            a.cardId, b.cardId, state, ctx, a.fromSeat, b.fromSeat, headSeat, n) < 0;
    });

    const int assignCount = std::min(static_cast<int>(pending.size()), static_cast<int>(recipientSeats.size()));
    for (int i = 0; i < assignCount; ++i) {
        const auto& tribute = pending[i];
        const int toSeat = recipientSeats[i];
        transferCard(state, tribute.fromSeat, toSeat, tribute.cardId);
        result.tributes.push_back({tribute.fromSeat, toSeat, tribute.cardId});
    }

    // 还贡：收到进贡牌的玩家还一张 10 及以下且非级牌
    for (const auto& tr : result.tributes) {
        CardId returnId = pickBestReturnCard(state.players[tr.toSeat].hand, state, ctx);
        if (returnId == 0) continue;
        transferCard(state, tr.toSeat, tr.fromSeat, returnId);
        result.returns.push_back({tr.toSeat, tr.fromSeat, returnId});
    }

    // 首出：给头游进贡的玩家；若头游未收到贡则头游首出
    result.headTributerSeat = -1;
    for (const auto& tr : result.tributes) {
        if (tr.toSeat == headSeat) {
            result.headTributerSeat = tr.fromSeat;
            break;
        }
    }
    if (result.headTributerSeat >= 0) {
        result.firstPlayerSeat = result.headTributerSeat;
    } else {
        result.firstPlayerSeat = headSeat;
    }

    return result;
}

}  // namespace guandan
