#include "game/card.h"

namespace guandan {

Card makeCard(CardId id, Suit suit, Rank rank) {
    return Card{id, suit, rank};
}

std::vector<Card> createFullDeck(int deckCount) {
    std::vector<Card> cards;
    cards.reserve(deckCount * 54);
    CardId id = 0;

    for (int d = 0; d < deckCount; ++d) {
        for (int s = 0; s < 4; ++s) {
            auto suit = static_cast<Suit>(s);
            for (int r = 2; r <= 14; ++r) {
                cards.push_back(makeCard(id++, suit, static_cast<Rank>(r)));
            }
        }
        cards.push_back(makeCard(id++, Suit::JOKER, Rank::SMALL_JOKER));
        cards.push_back(makeCard(id++, Suit::JOKER, Rank::BIG_JOKER));
    }
    return cards;
}

}  // namespace guandan
