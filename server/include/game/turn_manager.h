#pragma once

#include "game/game_state.h"

namespace guandan {

class TurnManager {
public:
    explicit TurnManager(GameState& state);

    int nextActivePlayer(int fromIndex) const;
    /// 接风：从出完牌的玩家起顺时针找第一个未出完的同队队友。
    int windPartnerSeat(int finishedLeaderSeat) const;
    /// 本轮领出座位：若上一手玩家已出完则交给顺位队友接风。
    int resolveRoundLeadSeat(int trickWinnerSeat) const;
    void advanceTurn();
    void resetRound(int winnerIndex);
    bool allOthersPassed() const;
    bool shouldResetRoundAfterPass(int seatIndex) const;

private:
    GameState& state_;
};

}  // namespace guandan
