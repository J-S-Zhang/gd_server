#include "game/deck.h"
#include "utils/random.h"
#include <algorithm>
#include <stdexcept>

/// @file deck.cpp
/// @brief 牌堆洗牌与发牌

namespace guandan {

/// 构造 Deck 实例
Deck::Deck(int deckCount) : cards_(createFullDeck(deckCount)) {}

/// 实现 shuffle 相关逻辑
void Deck::shuffle() {
    auto& rng = Random::instance().engine();
    std::shuffle(cards_.begin(), cards_.end(), rng);
}

/// 实现 shuffle 相关逻辑
void Deck::shuffle(uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::shuffle(cards_.begin(), cards_.end(), rng);
}

/// 实现 deal 相关逻辑
std::vector<std::vector<CardId>> Deck::deal(int playerCount, int cardsPerPlayer) {
    if (playerCount <= 0) {
        throw std::runtime_error("Invalid player count");
    }

    if (cardsPerPlayer > 0) {
        const int needed = playerCount * cardsPerPlayer;
        if (static_cast<int>(cards_.size()) < needed) {
            throw std::runtime_error("Not enough cards to deal");
        }
        std::vector<std::vector<CardId>> hands(playerCount);
        size_t cursor = 0;
        for (int p = 0; p < playerCount; ++p) {
            hands[p].reserve(cardsPerPlayer);
            for (int c = 0; c < cardsPerPlayer; ++c) {
                hands[p].push_back(cards_[cursor++].id);
            }
        }
        return hands;
    }

    if (static_cast<int>(cards_.size()) % playerCount != 0) {
        throw std::runtime_error("Cannot deal cards evenly");
    }
    int perPlayer = static_cast<int>(cards_.size()) / playerCount;
    std::vector<std::vector<CardId>> hands(playerCount);
    for (int p = 0; p < playerCount; ++p) {
        hands[p].reserve(perPlayer);
    }
    for (size_t i = 0; i < cards_.size(); ++i) {
        hands[i % playerCount].push_back(cards_[i].id);
    }
    return hands;
}

/// 实现 getCard 相关逻辑
Card Deck::getCard(CardId id) const {
    for (const auto& c : cards_) {
        if (c.id == id) return c;
    }
    throw std::runtime_error("Card not found: " + std::to_string(id));
}

}  // namespace guandan
