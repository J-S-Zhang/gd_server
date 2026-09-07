#pragma once

#include "game/card_pattern.h"
#include "game/types.h"

namespace guandan {

class RuleEngine {
public:
    bool canBeat(
        const CardPattern& current,
        const CardPattern& previous,
        const RuleContext& context
    ) const;

    bool isBombType(CardType type) const;

private:
    bool sameTypeBeat(const CardPattern& cur, const CardPattern& prev) const;
};

}  // namespace guandan
