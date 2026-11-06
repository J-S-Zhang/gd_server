#pragma once

#include "game/card.h"
#include "game/card_pattern.h"
#include "game/types.h"

namespace guandan {

/// 牌型大小比较（能否压过上家）
class RuleEngine {
public:
    /// current 能否压过 previous（首出时 previous 可为 invalid）
    bool canBeat(
        const CardPattern& current,
        const CardPattern& previous,
        const RuleContext& context
    ) const;

    /// 是否为炸弹类牌型
    bool isBombType(CardType type) const;

private:
    bool sameTypeBeat(const CardPattern& cur, const CardPattern& prev) const;
};

}  // namespace guandan
