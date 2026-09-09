#pragma once

#include "game/game_state.h"
#include <array>
#include <vector>

namespace guandan {

struct SettlementResult {
    int winningTeam = -1;       // 0=A, 1=B, -1=none
    int levelUpgrade = 0;
    int newLevel = 0;             // 胜方升级后的级牌
    std::array<int, 2> teamLevels{2, 2};
    std::array<bool, 2> inPassAPhase{false, false};
    std::array<int, 2> passAFailCounts{0, 0};
    int attackingTeam = 0;
    bool isPassARound = false;
    int passATeam = -1;
    bool passASuccess = false;
    bool matchWon = false;
    bool enteredPassAPhase = false;
    int passAFailCount = 0;
    bool passAFailReset = false;
    std::vector<int> ranks;     // player index -> finish rank
    std::vector<PlayerId> playerIds;
};

class Settlement {
public:
    SettlementResult calculate(const GameState& state, int playersPerTeam) const;

private:
    int findRank(const GameState& state, int rank) const;
    int sumWinningTeamRanks(const GameState& state, int team, int count) const;
};

}  // namespace guandan
