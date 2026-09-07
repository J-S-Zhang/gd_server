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
