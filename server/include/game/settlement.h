#pragma once

#include "game/game_state.h"
#include <array>
#include <vector>

namespace guandan {

/// 单局结算结果
struct SettlementResult {
    int winningTeam = -1;       ///< 胜队 0=A, 1=B, -1=无
    int levelUpgrade = 0;       ///< 升级数
    int newLevel = 0;           ///< 胜方升级后的级牌
    std::array<int, 2> teamLevels{2, 2};           ///< 两队当前级牌
    std::array<bool, 2> inPassAPhase{false, false};  ///< 是否处于过 A 阶段
    std::array<int, 2> passAFailCounts{0, 0};        ///< 过 A 失败次数
    int attackingTeam = 0;      ///< 攻方
    bool isPassARound = false;  ///< 是否过 A 局
    int passATeam = -1;         ///< 过 A 的目标队
    bool passASuccess = false;  ///< 过 A 是否成功
    bool matchWon = false;      ///< 是否整场胜利（如过 A 成功）
    bool enteredPassAPhase = false;  ///< 本局是否刚进入过 A 阶段
    int passAFailCount = 0;     ///< 本次过 A 相关失败计数
    bool passAFailReset = false;///< 过 A 失败计数是否被重置
    std::vector<int> ranks;     ///< 各座位 finish rank
    std::vector<PlayerId> playerIds;  ///< 与 ranks 对应的玩家 id
};

/// 根据出完顺序与规则计算升级、过 A 等
class Settlement {
public:
    /// 计算结算；playersPerTeam 用于游次求和规则
    SettlementResult calculate(const GameState& state, int playersPerTeam) const;

private:
    int findRank(const GameState& state, int rank) const;
    int sumWinningTeamRanks(const GameState& state, int team, int count) const;
};

}  // namespace guandan
