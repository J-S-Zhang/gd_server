#pragma once

#include "game/card.h"
#include "game/card_analyzer.h"
#include "game/game_state.h"
#include "game/rule_engine.h"
#include <optional>
#include <vector>

namespace guandan {

/// 测试房机器人：枚举手牌子集，选能压牌的最省牌组合；否则过牌。
class BotPlayer {
public:
    std::optional<std::vector<CardId>> choosePlay(
        const GameState& state,
        int seatIndex,
        const RuleContext& ctx,
        const CardAnalyzer& analyzer,
        const RuleEngine& rules
    ) const;

private:
    static int leadScore(size_t cardCount, const CardPattern& pattern);
    static int beatScore(size_t cardCount, const CardPattern& pattern);
};

}  // namespace guandan
