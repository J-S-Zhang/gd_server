#include "game/deck.h"
#include "utils/random.h"
#include <algorithm>
#include <stdexcept>

namespace guandan {

Deck::Deck(int deckCount) : cards_(createFullDeck(deckCount)) {}

void Deck::shuffle() {
    auto& rng = Random::instance().engine();
    std::shuffle(cards_.begin(), cards_.end(), rng);
}

void Deck::shuffle(uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::shuffle(cards_.begin(), cards_.end(), rng);
}

std::vector<std::vector<CardId>> Deck::deal(int playerCount) {
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

Card Deck::getCard(CardId id) const {
    for (const auto& c : cards_) {
        if (c.id == id) return c;
    }
    throw std::runtime_error("Card not found: " + std::to_string(id));
}

}  // namespace guandan
