#include "game/finish_rank.h"
#include "game/settlement.h"
#include "test_framework.h"

using namespace guandan;

namespace {

GameState makeFourPlayerPartialState(const std::array<int, 4>& finishRanks) {
    GameState state;
    state.playerCount = 4;
    for (int i = 0; i < 4; ++i) {
        state.players[i].id = 1000 + i;
        state.players[i].seatIndex = i;
        state.players[i].team = i % 2;
        state.players[i].finishRank = finishRanks[i];
        state.players[i].hasFinished = finishRanks[i] > 0;
    }
    return state;
}

}  // namespace

TEST(test_assign_remaining_finish_rank_four_player_one_left) {
    // 0=头游, 1=二游, 3=三游；2 未出完 → 逆时针从 3 的下家起应得末游
    auto state = makeFourPlayerPartialState({1, 2, 0, 3});
    assignRemainingFinishRanks(state);

    ASSERT(state.players[2].hasFinished);
    ASSERT(state.players[2].finishRank == 4);

    Settlement settlement;
    auto result = settlement.calculate(state, 2);
    ASSERT(result.winningTeam == 0);
    ASSERT(result.levelUpgrade == 1);
}

TEST(test_assign_remaining_finish_rank_counter_clockwise_order) {
    // 0=1, 2=2, 1 未出完, 3 未出完；最后出完为 seat2(二游)，逆时针 3→0→1
    auto state = makeFourPlayerPartialState({1, 0, 2, 0});
    assignRemainingFinishRanks(state);

    ASSERT(state.players[3].finishRank == 3);
    ASSERT(state.players[1].finishRank == 4);
}

TEST(test_assign_remaining_finish_rank_all_assigned_skips) {
    auto state = makeFourPlayerPartialState({1, 2, 3, 4});
    assignRemainingFinishRanks(state);
    ASSERT(state.players[0].finishRank == 1);
    ASSERT(state.players[3].finishRank == 4);
}
