#pragma once

#include "game/game_state.h"

namespace guandan {

/// 对局结束时，为尚未出完牌的玩家按逆时针补赋游次（从最后一名已出完者的下家起）。
void assignRemainingFinishRanks(GameState& state);

}  // namespace guandan
