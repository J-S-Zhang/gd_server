#include "game/settlement.h"
#include "game/team_progress.h"
#include "test_framework.h"

using namespace guandan;

namespace {

SettlementResult makeResult(int winningTeam, int levelUpgrade) {
    SettlementResult result;
    result.winningTeam = winningTeam;
    result.levelUpgrade = levelUpgrade;
    return result;
}

}  // namespace

TEST(test_enter_pass_a_phase_when_reaching_a) {
    TeamProgress progress;
    progress.levels[0] = 13;

    auto result = makeResult(0, 1);
    progress.applySettlement(result);

    ASSERT(progress.levels[0] == kLevelA);
    ASSERT(progress.inPassAPhase[0]);
    ASSERT(result.enteredPassAPhase);
}

TEST(test_pass_a_success) {
    TeamProgress progress;
    progress.levels[0] = kLevelA;
    progress.inPassAPhase[0] = true;
    progress.attackingTeam = 0;

    auto result = makeResult(0, 2);
    progress.applySettlement(result);

    ASSERT(result.isPassARound);
    ASSERT(result.passASuccess);
    ASSERT(result.matchWon);
    ASSERT(progress.passAFailCounts[0] == 0);
}

TEST(test_pass_a_fail_increments_count) {
    TeamProgress progress;
    progress.levels[0] = kLevelA;
    progress.inPassAPhase[0] = true;
    progress.attackingTeam = 0;

    auto result = makeResult(0, 0);
    progress.applySettlement(result);

    ASSERT(result.isPassARound);
    ASSERT(!result.passASuccess);
    ASSERT(progress.passAFailCounts[0] == 1);
    ASSERT(progress.levels[0] == kLevelA);
}

TEST(test_pass_a_fail_reset_after_three) {
    TeamProgress progress;
    progress.levels[0] = kLevelA;
    progress.inPassAPhase[0] = true;
    progress.passAFailCounts[0] = 2;
    progress.attackingTeam = 0;

    auto result = makeResult(1, 2);
    progress.applySettlement(result);

    ASSERT(result.passAFailReset);
    ASSERT(progress.levels[0] == 2);
    ASSERT(!progress.inPassAPhase[0]);
    ASSERT(progress.passAFailCounts[0] == 0);
}

TEST(test_attacking_team_follows_winner) {
    TeamProgress progress;
    auto result = makeResult(1, 2);
    progress.applySettlement(result);

    ASSERT(progress.attackingTeam == 1);
    ASSERT(progress.lastWinningTeam == 1);

    progress.prepareNextRound();
    ASSERT(progress.attackingTeam == 1);
    ASSERT(progress.currentRoundLevel() == progress.levels[1]);
}

TEST(test_opponent_round_not_pass_a) {
    TeamProgress progress;
    progress.levels[0] = kLevelA;
    progress.inPassAPhase[0] = true;
    progress.attackingTeam = 1;
    progress.levels[1] = 10;

    progress.prepareNextRound();
    ASSERT(!progress.isPassARound());
    ASSERT(progress.isPlayingOwnRound(1));
    ASSERT(!progress.isPlayingOwnRound(0));
}
