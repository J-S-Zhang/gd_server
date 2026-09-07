#include "game/game_state.h"
#include <stdexcept>

namespace guandan {

const Card& GameState::getCardById(CardId id) const {
    for (const auto& c : allCards) {
        if (c.id == id) return c;
    }
    throw std::runtime_error("Card id not found: " + std::to_string(id));
}

PlayerState* GameState::getPlayerById(PlayerId id) {
    for (auto& p : players) {
        if (p.id == id) return &p;
    }
    return nullptr;
}

const PlayerState* GameState::getPlayerById(PlayerId id) const {
    for (const auto& p : players) {
        if (p.id == id) return &p;
    }
    return nullptr;
}

}  // namespace guandan
