#pragma once

#include "game/settlement.h"
#include <array>

namespace guandan {

constexpr int kLevelA = 14;
constexpr int kPassAFailLimit = 3;

struct TeamProgress {
    std::array<int, 2> levels{2, 2};
    std::array<bool, 2> inPassAPhase{false, false};
    std::array<int, 2> passAFailCounts{0, 0};
    int attackingTeam = 0;
    int lastWinningTeam = -1;

    int currentRoundLevel() const { return levels[attackingTeam]; }

    bool isPassARound() const {
        return inPassAPhase[attackingTeam] && levels[attackingTeam] >= kLevelA;
    }

    bool isPlayingOwnRound(int team) const { return team == attackingTeam; }

    void prepareNextRound();

    void applySettlement(SettlementResult& result);
};

}  // namespace guandan
