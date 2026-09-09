#include "game/settlement.h"
#include "test_framework.h"

using namespace guandan;

namespace {

GameState makeFourPlayerState(const std::array<int, 4>& finishRanks) {
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

GameState makeSixPlayerState(const std::array<int, 6>& finishRanks) {
    GameState state;
    state.playerCount = 6;
    for (int i = 0; i < 6; ++i) {
        state.players[i].id = 2000 + i;
        state.players[i].seatIndex = i;
        state.players[i].team = i % 2;
        state.players[i].finishRank = finishRanks[i];
        state.players[i].hasFinished = finishRanks[i] > 0;
    }
    return state;
}

}  // namespace

TEST(test_settlement_four_player_upgrade) {
    Settlement settlement;

    auto state = makeFourPlayerState({1, 3, 2, 4});
    auto result = settlement.calculate(state, 2);
    ASSERT(result.winningTeam == 0);
    ASSERT(result.levelUpgrade == 3);

    state = makeFourPlayerState({1, 2, 3, 4});
    result = settlement.calculate(state, 2);
    ASSERT(result.winningTeam == 0);
    ASSERT(result.levelUpgrade == 2);

    state = makeFourPlayerState({1, 2, 4, 3});
    result = settlement.calculate(state, 2);
    ASSERT(result.winningTeam == 0);
    ASSERT(result.levelUpgrade == 1);
}

TEST(test_settlement_six_player_upgrade) {
    Settlement settlement;

    auto state = makeSixPlayerState({1, 4, 2, 5, 3, 6});
    auto result = settlement.calculate(state, 3);
    ASSERT(result.winningTeam == 0);
    ASSERT(result.levelUpgrade == 7);

    state = makeSixPlayerState({1, 2, 3, 4, 5, 6});
    result = settlement.calculate(state, 3);
    ASSERT(result.winningTeam == 0);
    ASSERT(result.levelUpgrade == 6);

    state = makeSixPlayerState({1, 2, 5, 3, 6, 4});
    result = settlement.calculate(state, 3);
    ASSERT(result.winningTeam == 0);
    ASSERT(result.levelUpgrade == 4);
}
