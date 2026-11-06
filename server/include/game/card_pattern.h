#pragma once

#include "game/types.h"
#include <vector>

namespace guandan {

/// 识别出的牌型结构
struct CardPattern {
    CardType type = CardType::INVALID;  ///< 牌型
    int primaryRank = 0;   ///< 主牌点数
    int length = 0;        ///< 顺子/连对/钢板长度 或 炸弹张数
    int pairRank = 0;      ///< 三带二的对子点数
    bool isValid = false;  ///< 是否合法牌型

    /// 返回无效牌型占位
    static CardPattern invalid() {
        return CardPattern{};
    }
};

}  // namespace guandan
