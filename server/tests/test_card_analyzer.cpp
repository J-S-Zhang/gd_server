#include "game/card.h"
#include "game/card_analyzer.h"
#include "test_framework.h"
#include <stdexcept>

using namespace guandan;

static Card findCard(const std::vector<Card>& deck, Suit suit, Rank rank, int nth = 0) {
    int count = 0;
    for (const auto& c : deck) {
        if (c.suit == suit && c.rank == rank) {
            if (count == nth) return c;
            count++;
        }
    }
    throw std::runtime_error("card not found");
}

TEST(test_analyze_single) {
    auto deck = createFullDeck(3);
    RuleContext ctx;
    ctx.currentLevel = 2;
    CardAnalyzer analyzer;

    auto c = findCard(deck, Suit::SPADE, Rank::R5);
    auto p = analyzer.analyze({c}, ctx);
    ASSERT(p.isValid);
    ASSERT(p.type == CardType::SINGLE);
    ASSERT(p.primaryRank == 5);
}

TEST(test_analyze_pair) {
    auto deck = createFullDeck(3);
    RuleContext ctx;
    CardAnalyzer analyzer;

    auto c1 = findCard(deck, Suit::SPADE, Rank::R7);
    auto c2 = findCard(deck, Suit::HEART, Rank::R7);
    auto p = analyzer.analyze({c1, c2}, ctx);
    ASSERT(p.isValid);
    ASSERT(p.type == CardType::PAIR);
}

TEST(test_analyze_bomb) {
    auto deck = createFullDeck(3);
    RuleContext ctx;
    CardAnalyzer analyzer;

    std::vector<Card> cards;
    cards.push_back(findCard(deck, Suit::SPADE, Rank::R9, 0));
    cards.push_back(findCard(deck, Suit::HEART, Rank::R9, 0));
    cards.push_back(findCard(deck, Suit::CLUB, Rank::R9, 0));
    cards.push_back(findCard(deck, Suit::DIAMOND, Rank::R9, 0));
    auto p = analyzer.analyze(cards, ctx);
    ASSERT(p.isValid);
    ASSERT(p.type == CardType::BOMB);
    ASSERT(p.length == 4);
}

TEST(test_analyze_three_pairs_a2233) {
    auto deck = createFullDeck(3);
    RuleContext ctx;
    CardAnalyzer analyzer;

    std::vector<Card> cards;
    cards.push_back(findCard(deck, Suit::SPADE, Rank::A));
    cards.push_back(findCard(deck, Suit::HEART, Rank::A, 0));
    cards.push_back(findCard(deck, Suit::CLUB, Rank::R2));
    cards.push_back(findCard(deck, Suit::DIAMOND, Rank::R2, 0));
    cards.push_back(findCard(deck, Suit::SPADE, Rank::R3));
    cards.push_back(findCard(deck, Suit::HEART, Rank::R3, 0));
    auto p = analyzer.analyze(cards, ctx);
    ASSERT(p.isValid);
    ASSERT(p.type == CardType::THREE_PAIRS);
    ASSERT(p.primaryRank == 3);
}

TEST(test_analyze_two_triples_a222) {
    auto deck = createFullDeck(3);
    RuleContext ctx;
    CardAnalyzer analyzer;

    std::vector<Card> cards;
    cards.push_back(findCard(deck, Suit::SPADE, Rank::A));
    cards.push_back(findCard(deck, Suit::HEART, Rank::A, 0));
    cards.push_back(findCard(deck, Suit::CLUB, Rank::A, 1));
    cards.push_back(findCard(deck, Suit::DIAMOND, Rank::R2));
    cards.push_back(findCard(deck, Suit::SPADE, Rank::R2, 0));
    cards.push_back(findCard(deck, Suit::HEART, Rank::R2, 1));
    auto p = analyzer.analyze(cards, ctx);
    ASSERT(p.isValid);
    ASSERT(p.type == CardType::TWO_TRIPLES);
    ASSERT(p.primaryRank == 2);
}

TEST(test_analyze_straight_a2345) {
    auto deck = createFullDeck(3);
    RuleContext ctx;
    CardAnalyzer analyzer;

    std::vector<Card> cards;
    cards.push_back(findCard(deck, Suit::SPADE, Rank::A));
    cards.push_back(findCard(deck, Suit::HEART, Rank::R2));
    cards.push_back(findCard(deck, Suit::CLUB, Rank::R3));
    cards.push_back(findCard(deck, Suit::DIAMOND, Rank::R4));
    cards.push_back(findCard(deck, Suit::SPADE, Rank::R5));
    auto p = analyzer.analyze(cards, ctx);
    ASSERT(p.isValid);
    ASSERT(p.type == CardType::STRAIGHT);
    ASSERT(p.primaryRank == 5);
}

TEST(test_analyze_invalid) {
    auto deck = createFullDeck(3);
    RuleContext ctx;
    CardAnalyzer analyzer;

    auto c1 = findCard(deck, Suit::SPADE, Rank::R3);
    auto c2 = findCard(deck, Suit::HEART, Rank::R5);
    auto p = analyzer.analyze({c1, c2}, ctx);
    ASSERT(!p.isValid);
}
