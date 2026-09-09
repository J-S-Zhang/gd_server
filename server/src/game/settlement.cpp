#include "game/settlement.h"
#include <algorithm>
#include <vector>

namespace guandan {

int Settlement::findRank(const GameState& state, int rank) const {
    for (int i = 0; i < state.playerCount; ++i) {
        if (state.players[i].finishRank == rank) return i;
    }
    return -1;
}

int Settlement::sumWinningTeamRanks(const GameState& state, int team, int count) const {
    std::vector<int> ranks;
    ranks.reserve(count);
    for (int i = 0; i < state.playerCount; ++i) {
        if (state.players[i].team == team && state.players[i].finishRank > 0) {
            ranks.push_back(state.players[i].finishRank);
        }
    }
    std::sort(ranks.begin(), ranks.end());
    const int take = std::min(count, static_cast<int>(ranks.size()));
    int sum = 0;
    for (int i = 0; i < take; ++i) {
        sum += ranks[i];
    }
    return sum;
}

SettlementResult Settlement::calculate(const GameState& state, int playersPerTeam) const {
    SettlementResult result;
    result.ranks.resize(state.playerCount, 0);
    result.playerIds.resize(state.playerCount);

    for (int i = 0; i < state.playerCount; ++i) {
        result.ranks[i] = state.players[i].finishRank;
        result.playerIds[i] = state.players[i].id;
    }

    const int headIdx = findRank(state, 1);
    if (headIdx < 0) return result;

    const int winningTeam = state.players[headIdx].team;
    result.winningTeam = winningTeam;

    const int rankCount = playersPerTeam >= 3 ? 2 : playersPerTeam;
    const int rankSum = sumWinningTeamRanks(state, winningTeam, rankCount);
    const int base = playersPerTeam >= 3 ? 10 : 6;
    result.levelUpgrade = base - rankSum;

    return result;
}

}  // namespace guandan
