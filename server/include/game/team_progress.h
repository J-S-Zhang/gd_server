#pragma once

#include "game/settlement.h"
#include <array>

namespace guandan {

constexpr int kLevelA = 14;           ///< 级牌 A 的数值
constexpr int kPassAFailLimit = 3;    ///< 过 A 允许失败次数上限

/// 跨局队伍进度（级牌、过 A）
struct TeamProgress {
    std::array<int, 2> levels{2, 2};                 ///< 两队级牌
    std::array<bool, 2> inPassAPhase{false, false};  ///< 是否在过 A 阶段
    std::array<int, 2> passAFailCounts{0, 0};        ///< 过 A 失败次数
    int attackingTeam = 0;                           ///< 当前攻方
    int lastWinningTeam = -1;                        ///< 上一局胜队

    /// 当前局打的级牌（攻方级牌）
    int currentRoundLevel() const { return levels[attackingTeam]; }

    /// 是否为过 A 局
    bool isPassARound() const {
        return inPassAPhase[attackingTeam] && levels[attackingTeam] >= kLevelA;
    }

    /// 某队是否为本局攻方（打自己的级）
    bool isPlayingOwnRound(int team) const { return team == attackingTeam; }

    /// 准备下一局（切换攻方等）
    void prepareNextRound();

    /// 将结算结果写回进度
    void applySettlement(SettlementResult& result);
};

}  // namespace guandan
