#include "game/card.h"
#include "game/card_analyzer.h"
#include "game/rule_engine.h"
#include "test_framework.h"

using namespace guandan;

TEST(test_pair_beats_pair) {
    RuleEngine engine;
    RuleContext ctx;

    CardPattern higher;
    higher.type = CardType::PAIR;
    higher.primaryRank = 8;
    higher.length = 2;
    higher.isValid = true;

    CardPattern lower;
    lower.type = CardType::PAIR;
    lower.primaryRank = 5;
    lower.length = 2;
    lower.isValid = true;

    ASSERT(engine.canBeat(higher, lower, ctx));
    ASSERT(!engine.canBeat(lower, higher, ctx));
}

TEST(test_bomb_beats_pair) {
    RuleEngine engine;
    RuleContext ctx;

    CardPattern bomb;
    bomb.type = CardType::BOMB;
    bomb.primaryRank = 3;
    bomb.length = 4;
    bomb.isValid = true;

    CardPattern pair;
    pair.type = CardType::PAIR;
    pair.primaryRank = 14;
    pair.length = 2;
    pair.isValid = true;

    ASSERT(engine.canBeat(bomb, pair, ctx));
    ASSERT(!engine.canBeat(pair, bomb, ctx));
}

TEST(test_bigger_bomb) {
    RuleEngine engine;
    RuleContext ctx;

    CardPattern bomb5;
    bomb5.type = CardType::BOMB;
    bomb5.primaryRank = 7;
    bomb5.length = 5;
    bomb5.isValid = true;

    CardPattern bomb4;
    bomb4.type = CardType::BOMB;
    bomb4.primaryRank = 14;
    bomb4.length = 4;
    bomb4.isValid = true;

    ASSERT(engine.canBeat(bomb5, bomb4, ctx));
}

TEST(test_new_round) {
    RuleEngine engine;
    RuleContext ctx;

    CardPattern any;
    any.type = CardType::SINGLE;
    any.primaryRank = 3;
    any.isValid = true;

    CardPattern none = CardPattern::invalid();
    ASSERT(engine.canBeat(any, none, ctx));
}

TEST(test_straight_flush_beats_five_bomb) {
    RuleEngine engine;
    RuleContext ctx;

    CardPattern sf;
    sf.type = CardType::STRAIGHT_FLUSH;
    sf.primaryRank = 7;
    sf.length = 5;
    sf.isValid = true;

    CardPattern bomb5;
    bomb5.type = CardType::BOMB;
    bomb5.primaryRank = 14;
    bomb5.length = 5;
    bomb5.isValid = true;

    ASSERT(engine.canBeat(sf, bomb5, ctx));
    ASSERT(!engine.canBeat(bomb5, sf, ctx));
}

TEST(test_six_bomb_beats_straight_flush) {
    RuleEngine engine;
    RuleContext ctx;

    CardPattern bomb6;
    bomb6.type = CardType::BOMB;
    bomb6.primaryRank = 7;
    bomb6.length = 6;
    bomb6.isValid = true;

    CardPattern sf;
    sf.type = CardType::STRAIGHT_FLUSH;
    sf.primaryRank = 14;
    sf.length = 5;
    sf.isValid = true;

    ASSERT(engine.canBeat(bomb6, sf, ctx));
    ASSERT(!engine.canBeat(sf, bomb6, ctx));
}

TEST(test_a2345_straight_smallest) {
    RuleEngine engine;
    RuleContext ctx;

    CardPattern low;
    low.type = CardType::STRAIGHT;
    low.primaryRank = 5;
    low.length = 5;
    low.isValid = true;

    CardPattern high;
    high.type = CardType::STRAIGHT;
    high.primaryRank = 6;
    high.length = 5;
    high.isValid = true;

    ASSERT(engine.canBeat(high, low, ctx));
    ASSERT(!engine.canBeat(low, high, ctx));
}

TEST(test_straight_flush_beats_non_bomb) {
    RuleEngine engine;
    RuleContext ctx;

    CardPattern sf;
    sf.type = CardType::STRAIGHT_FLUSH;
    sf.primaryRank = 7;
    sf.length = 5;
    sf.isValid = true;

    CardPattern straight;
    straight.type = CardType::STRAIGHT;
    straight.primaryRank = 14;
    straight.length = 5;
    straight.isValid = true;

    ASSERT(engine.canBeat(sf, straight, ctx));
    ASSERT(!engine.canBeat(straight, sf, ctx));
}
