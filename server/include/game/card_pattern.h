#pragma once

#include "game/types.h"
#include <vector>

namespace guandan {

struct CardPattern {
    CardType type = CardType::INVALID;
    int primaryRank = 0;   // 主牌点数
    int length = 0;        // 顺子/连对/钢板长度 或 炸弹张数
    int pairRank = 0;      // 三带二的对子点数
    bool isValid = false;

    static CardPattern invalid() {
        return CardPattern{};
    }
};

}  // namespace guandan
