#pragma once

#include "game/game_state.h"
#include <vector>

namespace guandan {

struct SettlementResult {
    int winningTeam = -1;       // 0=A, 1=B, -1=none
    int levelUpgrade = 0;
    std::vector<int> ranks;     // player index -> finish rank
    std::vector<PlayerId> playerIds;
};

class Settlement {
public:
    SettlementResult calculate(const GameState& state) const;

private:
    int countTeamFinished(const GameState& state, int team) const;
    int findRank(const GameState& state, int rank) const;
};

}  // namespace guandan
