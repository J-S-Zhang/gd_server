#include "game/card.h"
#include "game/deck.h"
#include "test_framework.h"

using namespace guandan;

TEST(test_create_full_deck) {
    auto cards = createFullDeck(3);
    ASSERT(cards.size() == 162);
}

TEST(test_deck_deal) {
    Deck deck(3);
    deck.shuffle(42);
    auto hands = deck.deal(6);
    ASSERT(hands.size() == 6);
    int total = 0;
    for (const auto& h : hands) total += static_cast<int>(h.size());
    ASSERT(total == 162);
    for (const auto& h : hands) ASSERT(h.size() == 27);
}

TEST(test_deck_deal_fixed_count) {
    Deck deck(2);
    deck.shuffle(42);
    auto hands = deck.deal(4, 10);
    ASSERT(hands.size() == 4);
    for (const auto& h : hands) {
        ASSERT(h.size() == 10);
    }
}

TEST(test_card_string) {
    auto c = makeCard(0, Suit::HEART, Rank::R5);
    ASSERT(c.toString() == "5H");
}
