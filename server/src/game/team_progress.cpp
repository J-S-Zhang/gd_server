#include "game/team_progress.h"
#include <algorithm>

namespace guandan {

void TeamProgress::prepareNextRound() {
    if (lastWinningTeam >= 0) {
        attackingTeam = lastWinningTeam;
    } else {
        attackingTeam = 0;
    }
}

void TeamProgress::applySettlement(SettlementResult& result) {
    result.teamLevels = levels;
    result.inPassAPhase = inPassAPhase;
    result.passAFailCounts = passAFailCounts;
    result.attackingTeam = attackingTeam;

    if (result.winningTeam < 0) return;

    const bool passARound = isPassARound();
    result.isPassARound = passARound;

    if (passARound) {
        const int passTeam = attackingTeam;
        result.passATeam = passTeam;
        const bool success = result.winningTeam == passTeam && result.levelUpgrade > 0;
        result.passASuccess = success;

        if (success) {
            result.matchWon = true;
            result.newLevel = levels[passTeam];
        } else {
            passAFailCounts[passTeam]++;
            result.passAFailCount = passAFailCounts[passTeam];
            if (passAFailCounts[passTeam] >= kPassAFailLimit) {
                levels[passTeam] = 2;
                inPassAPhase[passTeam] = false;
                passAFailCounts[passTeam] = 0;
                result.passAFailReset = true;
            }
            result.newLevel = levels[passTeam];
        }
    } else if (result.levelUpgrade > 0) {
        int& level = levels[result.winningTeam];
        const int upgraded = level + result.levelUpgrade;
        if (!inPassAPhase[result.winningTeam] && upgraded >= kLevelA) {
            inPassAPhase[result.winningTeam] = true;
            level = kLevelA;
            result.enteredPassAPhase = true;
        } else {
            level = std::min(kLevelA, upgraded);
        }
        result.newLevel = level;
    }

    lastWinningTeam = result.winningTeam;
    attackingTeam = lastWinningTeam;

    result.teamLevels = levels;
    result.inPassAPhase = inPassAPhase;
    result.passAFailCounts = passAFailCounts;
    result.attackingTeam = attackingTeam;
}

}  // namespace guandan
