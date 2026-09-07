#include "game/settlement.h"

namespace guandan {

int Settlement::countTeamFinished(const GameState& state, int team) const {
    int count = 0;
    for (const auto& p : state.players) {
        if (p.team == team && p.hasFinished) ++count;
    }
    return count;
}

int Settlement::findRank(const GameState& state, int rank) const {
    for (int i = 0; i < state.playerCount; ++i) {
        if (state.players[i].finishRank == rank) return i;
    }
    return -1;
}

SettlementResult Settlement::calculate(const GameState& state) const {
    SettlementResult result;
    result.ranks.resize(state.playerCount, 0);
    result.playerIds.resize(state.playerCount);

    for (int i = 0; i < state.playerCount; ++i) {
        result.ranks[i] = state.players[i].finishRank;
        result.playerIds[i] = state.players[i].id;
    }

    int headIdx = findRank(state, 1);
    if (headIdx < 0) return result;

    int headTeam = state.players[headIdx].team;
    int secondIdx = findRank(state, 2);
    int thirdIdx = findRank(state, 3);

    if (secondIdx >= 0 && state.players[secondIdx].team == headTeam) {
        result.winningTeam = headTeam;
        result.levelUpgrade = 3;  // 双下
    } else if (thirdIdx >= 0 && state.players[thirdIdx].team == headTeam) {
        result.winningTeam = headTeam;
        result.levelUpgrade = 2;  // 单下
    } else {
        result.winningTeam = headTeam;
        result.levelUpgrade = 1;
    }

    return result;
}

}  // namespace guandan
