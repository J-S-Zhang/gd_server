#pragma once

#include "game/game_state.h"

namespace guandan {

class TurnManager {
public:
    explicit TurnManager(GameState& state);

    int nextActivePlayer(int fromIndex) const;
    void advanceTurn();
    void resetRound(int winnerIndex);
    bool allOthersPassed() const;
    bool shouldResetRoundAfterPass(int seatIndex) const;

private:
    GameState& state_;
};

}  // namespace guandan
