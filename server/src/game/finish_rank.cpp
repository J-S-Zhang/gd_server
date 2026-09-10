#include "game/finish_rank.h"

namespace guandan {

void assignRemainingFinishRanks(GameState& state) {
    if (state.playerCount <= 0) return;

    int lastFinishedSeat = -1;
    int maxRank = 0;
    for (int i = 0; i < state.playerCount; ++i) {
        const auto& player = state.players[i];
        if (!player.hasFinished) continue;
        if (player.finishRank > maxRank) {
            maxRank = player.finishRank;
            lastFinishedSeat = i;
        }
    }
    if (lastFinishedSeat < 0 || maxRank <= 0) return;

    int nextRank = maxRank + 1;
    int seat = (lastFinishedSeat + 1) % state.playerCount;
    for (int step = 0; step < state.playerCount && nextRank <= state.playerCount; ++step) {
        auto& player = state.players[seat];
        if (!player.hasFinished) {
            player.hasFinished = true;
            player.finishRank = nextRank++;
        }
        seat = (seat + 1) % state.playerCount;
    }
}

}  // namespace guandan
