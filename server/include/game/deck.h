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

    /// cardsPerPlayer <= 0：整副牌均分；> 0：洗牌后每人随机发固定张数。
    std::vector<std::vector<CardId>> deal(int playerCount, int cardsPerPlayer = 0);

    const std::vector<Card>& cards() const { return cards_; }
    Card getCard(CardId id) const;

private:
    std::vector<Card> cards_;
};

}  // namespace guandan
