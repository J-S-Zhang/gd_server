#include "game/team_progress.h"
#include <algorithm>

/// @file team_progress.cpp
/// @brief 双方级数进度计算

namespace guandan {

/// 实现 prepareNextRound 相关逻辑
void TeamProgress::prepareNextRound() {
    // 有上一局胜方则以其为攻方；否则保留当前 attackingTeam（勿强行改回 0）
    if (lastWinningTeam >= 0) {
        attackingTeam = lastWinningTeam;
    }
}

/// 实现 applySettlement 相关逻辑
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
