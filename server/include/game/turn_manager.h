#pragma once

#include "game/game_state.h"

namespace guandan {

/// 出牌顺序、接风与一轮重置
class TurnManager {
public:
    explicit TurnManager(GameState& state);

    /// 从 fromIndex 起顺时针下一个未出完牌的玩家座位
    int nextActivePlayer(int fromIndex) const;
    /// 接风：从出完牌的玩家起顺时针找第一个未出完的同队队友。
    int windPartnerSeat(int finishedLeaderSeat) const;
    /// 本轮领出座位：若上一手玩家已出完则交给顺位队友接风。
    int resolveRoundLeadSeat(int trickWinnerSeat) const;
    /// 轮到下一个活跃玩家
    void advanceTurn();
    /// 新一轮：winnerIndex 领出并重置 pass 计数
    void resetRound(int winnerIndex);
    /// 除当前玩家外是否都已 pass
    bool allOthersPassed() const;
    /// pass 后是否应结束本轮（如接风场景）
    bool shouldResetRoundAfterPass(int seatIndex) const;

private:
    GameState& state_;  ///< 关联对局状态
};

}  // namespace guandan
