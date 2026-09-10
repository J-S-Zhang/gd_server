#include "game/bot_player.h"
#include "game/card.h"
#include "test_framework.h"

using namespace guandan;

namespace {

GameState makeStateWithHand(
    const std::vector<Card>& cards,
    const CardPattern& lastPattern,
    int lastPlayedSeat = 0
) {
    GameState state;
    state.playerCount = 2;
    state.players[0].id = 1001;
    state.players[0].seatIndex = 0;
    state.players[1].id = 1002;
    state.players[1].seatIndex = 1;

    CardId nextId = 1;
    for (const Card& card : cards) {
        Card stored = card;
        stored.id = nextId++;
        state.allCards.push_back(stored);
        state.players[0].hand.add(stored.id);
    }

    state.lastPattern = lastPattern;
    state.lastPlayedPlayerIndex = lastPlayedSeat;
    return state;
}

}  // namespace

TEST(test_bot_leads_smallest_single) {
    BotPlayer bot;
    CardAnalyzer analyzer;
    RuleEngine rules;
    RuleContext ctx;

    auto state = makeStateWithHand(
        {
            makeCard(0, Suit::SPADE, Rank::R5),
            makeCard(0, Suit::HEART, Rank::K),
        },
        CardPattern::invalid());

    auto play = bot.choosePlay(state, 0, ctx, analyzer, rules);
    ASSERT(play.has_value());
    ASSERT(play->size() == 1);
    ASSERT(state.getCardById((*play)[0]).rank == Rank::R5);
}

TEST(test_bot_beats_with_higher_single) {
    BotPlayer bot;
    CardAnalyzer analyzer;
    RuleEngine rules;
    RuleContext ctx;

    CardPattern last;
    last.type = CardType::SINGLE;
    last.primaryRank = rankValue(Rank::R5);
    last.length = 1;
    last.isValid = true;

    auto state = makeStateWithHand(
        {
            makeCard(0, Suit::SPADE, Rank::R5),
            makeCard(0, Suit::HEART, Rank::K),
        },
        last);

    auto play = bot.choosePlay(state, 0, ctx, analyzer, rules);
    ASSERT(play.has_value());
    ASSERT(play->size() == 1);
    ASSERT(state.getCardById((*play)[0]).rank == Rank::K);
}

TEST(test_bot_passes_when_cannot_beat) {
    BotPlayer bot;
    CardAnalyzer analyzer;
    RuleEngine rules;
    RuleContext ctx;

    CardPattern last;
    last.type = CardType::SINGLE;
    last.primaryRank = rankValue(Rank::A);
    last.length = 1;
    last.isValid = true;

    auto state = makeStateWithHand(
        {makeCard(0, Suit::SPADE, Rank::R5)},
        last);

    auto play = bot.choosePlay(state, 0, ctx, analyzer, rules);
    ASSERT(!play.has_value());
}
