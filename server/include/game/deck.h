#pragma once

#include "game/card.h"
#include <random>
#include <vector>

namespace guandan {

class Deck {
public:
    explicit Deck(int deckCount = kDeckCount);

    void shuffle();
    void shuffle(uint64_t seed);

    std::vector<std::vector<CardId>> deal(int playerCount);

    const std::vector<Card>& cards() const { return cards_; }
    Card getCard(CardId id) const;

private:
    std::vector<Card> cards_;
};

}  // namespace guandan
