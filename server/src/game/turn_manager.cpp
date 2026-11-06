#include "game/turn_manager.h"

/// @file turn_manager.cpp
/// @brief 回合轮转与过牌逻辑

namespace guandan {

/// 构造 TurnManager 实例
TurnManager::TurnManager(GameState& state) : state_(state) {}

/// 实现 nextActivePlayer 相关逻辑
int TurnManager::nextActivePlayer(int fromIndex) const {
    for (int i = 1; i <= state_.playerCount; ++i) {
        int idx = (fromIndex + i) % state_.playerCount;
        if (!state_.players[idx].hasFinished) return idx;
    }
    return fromIndex;
}

/// 实现 advanceTurn 相关逻辑
void TurnManager::advanceTurn() {
    state_.currentPlayerIndex = nextActivePlayer(state_.currentPlayerIndex);
    state_.turnId++;
}

/// 实现 windPartnerSeat 相关逻辑
int TurnManager::windPartnerSeat(int finishedLeaderSeat) const {
    if (finishedLeaderSeat < 0 || finishedLeaderSeat >= state_.playerCount) {
        return nextActivePlayer(state_.currentPlayerIndex);
    }

    const int team = state_.players[finishedLeaderSeat].team;
    for (int i = 1; i <= state_.playerCount; ++i) {
        const int idx = (finishedLeaderSeat + i) % state_.playerCount;
        if (state_.players[idx].hasFinished) continue;
        if (state_.players[idx].team == team) return idx;
    }

    return nextActivePlayer(finishedLeaderSeat);
}

/// 实现 resolveRoundLeadSeat 相关逻辑
int TurnManager::resolveRoundLeadSeat(int trickWinnerSeat) const {
    if (trickWinnerSeat < 0) {
        return nextActivePlayer(state_.currentPlayerIndex);
    }
    if (state_.players[trickWinnerSeat].hasFinished) {
        return windPartnerSeat(trickWinnerSeat);
    }
    return trickWinnerSeat;
}

/// 实现 resetRound 相关逻辑
void TurnManager::resetRound(int winnerIndex) {
    state_.lastPlayedCards.clear();
    state_.lastPattern = CardPattern::invalid();
    state_.lastPlayedPlayerIndex = -1;
    state_.passCount = 0;
    state_.currentPlayerIndex = resolveRoundLeadSeat(winnerIndex);
    state_.turnId++;
}

/// 实现 allOthersPassed 相关逻辑
bool TurnManager::allOthersPassed() const {
    int responders = 0;
    for (int i = 0; i < state_.playerCount; ++i) {
        if (state_.players[i].hasFinished) continue;
        if (i == state_.lastPlayedPlayerIndex) continue;
        ++responders;
    }
    return responders > 0 && state_.passCount >= responders;
}

/// 实现 shouldResetRoundAfterPass 相关逻辑
bool TurnManager::shouldResetRoundAfterPass(int seatIndex) const {
    if (state_.lastPlayedPlayerIndex < 0) return false;
    return nextActivePlayer(seatIndex) == state_.lastPlayedPlayerIndex;
}

}  // namespace guandan
