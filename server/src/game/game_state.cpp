#include "game/game_state.h"
#include <stdexcept>
#include <string>

/// @file game_state.cpp
/// @brief 对局运行时状态数据结构

namespace guandan {

/// 实现 getCardById 相关逻辑
const Card& GameState::getCardById(CardId id) const {
    for (const auto& c : allCards) {
        if (c.id == id) return c;
    }
    throw std::runtime_error("Card id not found: " + std::to_string(id));
}

/// 实现 getPlayerById 相关逻辑
PlayerState* GameState::getPlayerById(PlayerId id) {
    for (auto& p : players) {
        if (p.id == id) return &p;
    }
    return nullptr;
}

/// 实现 getPlayerById 相关逻辑
const PlayerState* GameState::getPlayerById(PlayerId id) const {
    for (const auto& p : players) {
        if (p.id == id) return &p;
    }
    return nullptr;
}

}  // namespace guandan
