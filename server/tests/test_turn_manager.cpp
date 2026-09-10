#include "game/turn_manager.h"
#include "test_framework.h"

using namespace guandan;

namespace {

GameState makeState(int playerCount) {
    GameState state;
    state.playerCount = playerCount;
    for (int i = 0; i < playerCount; ++i) {
        state.players[i].id = 1000 + i;
        state.players[i].seatIndex = i;
        state.players[i].team = i % 2;
    }
    return state;
}

}  // namespace

TEST(test_wind_partner_four_player) {
    GameState state = makeState(4);
    state.players[0].hasFinished = true;
    TurnManager turn(state);

    ASSERT(turn.windPartnerSeat(0) == 2);
    ASSERT(turn.resolveRoundLeadSeat(0) == 2);
}

TEST(test_wind_partner_six_player) {
    GameState state = makeState(6);
    state.players[0].hasFinished = true;
    TurnManager turn(state);

    ASSERT(turn.windPartnerSeat(0) == 2);
}

TEST(test_wind_partner_skips_finished_teammate) {
    GameState state = makeState(6);
    state.players[0].hasFinished = true;
    state.players[2].hasFinished = true;
    TurnManager turn(state);

    ASSERT(turn.windPartnerSeat(0) == 4);
}

TEST(test_reset_round_keeps_active_leader) {
    GameState state = makeState(4);
    state.lastPlayedPlayerIndex = 1;
    state.lastPattern = CardPattern{};
    state.lastPattern.isValid = true;
    TurnManager turn(state);

    turn.resetRound(1);
    ASSERT(state.currentPlayerIndex == 1);
    ASSERT(!state.lastPattern.isValid);
}

TEST(test_reset_round_wind_after_finish) {
    GameState state = makeState(4);
    state.players[1].hasFinished = true;
    state.lastPlayedPlayerIndex = 1;
    TurnManager turn(state);

    turn.resetRound(1);
    ASSERT(state.currentPlayerIndex == 3);
}
