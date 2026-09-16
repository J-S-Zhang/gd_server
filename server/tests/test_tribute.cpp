#include "game/tribute.h"
#include "test_framework.h"

using namespace guandan;

namespace {

GameState makeStateWithHands(
    int playerCount,
    const std::array<int, 6>& finishRanks,
    const std::vector<std::vector<Card>>& handsBySeat
) {
    GameState state;
    state.playerCount = playerCount;
    CardId nextId = 1;
    for (int seat = 0; seat < playerCount; ++seat) {
        state.players[seat].id = 1000 + seat;
        state.players[seat].seatIndex = seat;
        state.players[seat].team = seat % 2;
        state.players[seat].finishRank = finishRanks[seat];
        state.players[seat].hasFinished = finishRanks[seat] > 0;
        for (const Card& card : handsBySeat[seat]) {
            Card stored = card;
            stored.id = nextId++;
            state.allCards.push_back(stored);
            state.players[seat].hand.add(stored.id);
        }
    }
    return state;
}

PreviousRoundInfo previousFromState(const GameState& state, int winningTeam) {
    return buildPreviousRoundInfo(state, winningTeam);
}

}  // namespace

TEST(test_tribute_four_single_down) {
    TributeManager manager;
    GameRuleConfig config;
    config.playerCount = 4;
    config.playersPerTeam = 2;
    config.enableTribute = true;

    std::array<int, 6> ranks{1, 3, 2, 4, 0, 0};
    std::vector<std::vector<Card>> hands(6);
    hands[3] = {
        makeCard(0, Suit::SPADE, Rank::K),
        makeCard(0, Suit::HEART, Rank::R5),
        makeCard(0, Suit::CLUB, Rank::R2),
    };
    hands[0] = {makeCard(0, Suit::DIAMOND, Rank::R3)};

    GameState state = makeStateWithHands(4, ranks, hands);
    PreviousRoundInfo previous = previousFromState(state, 0);
    RuleContext ctx;
    ctx.currentLevel = 2;

    auto result = manager.resolveRound(state, config, previous, ctx);
    ASSERT(!result.skipped);
    ASSERT(!result.antiTribute);
    ASSERT(result.tributes.size() == 1);
    ASSERT(result.tributes[0].fromSeat == 3);
    ASSERT(result.tributes[0].toSeat == 0);
    ASSERT(state.getCardById(result.tributes[0].cardId).rank == Rank::K);
    ASSERT(result.returns.size() == 1);
    ASSERT(result.returns[0].fromSeat == 0);
    ASSERT(result.returns[0].toSeat == 3);
    ASSERT(result.firstPlayerSeat == 3);
}

TEST(test_tribute_four_double_down) {
    TributeManager manager;
    GameRuleConfig config;
    config.playerCount = 4;
    config.playersPerTeam = 2;
    config.enableTribute = true;

    std::array<int, 6> ranks{1, 2, 3, 4, 0, 0};
    std::vector<std::vector<Card>> hands(6);
    hands[2] = {makeCard(0, Suit::SPADE, Rank::Q)};
    hands[3] = {makeCard(0, Suit::CLUB, Rank::R10)};
    hands[0] = {makeCard(0, Suit::DIAMOND, Rank::R5), makeCard(0, Suit::HEART, Rank::R3)};
    hands[1] = {makeCard(0, Suit::DIAMOND, Rank::R4)};

    GameState state = makeStateWithHands(4, ranks, hands);
    PreviousRoundInfo previous = previousFromState(state, 0);
    RuleContext ctx;

    auto result = manager.resolveRound(state, config, previous, ctx);
    ASSERT(result.tributes.size() == 2);
    ASSERT(result.tributes[0].toSeat == 0);
    ASSERT(result.tributes[1].toSeat == 1);
    ASSERT(state.getCardById(result.tributes[0].cardId).rank == Rank::Q);
    ASSERT(state.getCardById(result.tributes[1].cardId).rank == Rank::R10);
    ASSERT(result.firstPlayerSeat == 2);
}

TEST(test_tribute_four_anti_double_big_jokers) {
    TributeManager manager;
    GameRuleConfig config;
    config.playerCount = 4;
    config.playersPerTeam = 2;
    config.enableTribute = true;

    std::array<int, 6> ranks{1, 2, 3, 4, 0, 0};
    std::vector<std::vector<Card>> hands(6);
    hands[2] = {
        makeCard(0, Suit::JOKER, Rank::BIG_JOKER),
        makeCard(0, Suit::JOKER, Rank::BIG_JOKER),
        makeCard(0, Suit::SPADE, Rank::A),
    };
    hands[3] = {makeCard(0, Suit::CLUB, Rank::K)};

    GameState state = makeStateWithHands(4, ranks, hands);
    PreviousRoundInfo previous = previousFromState(state, 0);
    RuleContext ctx;

    auto result = manager.resolveRound(state, config, previous, ctx);
    ASSERT(result.antiTribute);
    ASSERT(result.tributes.empty());
    ASSERT(result.firstPlayerSeat == 0);
}

TEST(test_tribute_skip_wild_card) {
    TributeManager manager;
    GameRuleConfig config;
    config.playerCount = 4;
    config.playersPerTeam = 2;
    config.enableTribute = true;

    std::array<int, 6> ranks{1, 3, 2, 4, 0, 0};
    std::vector<std::vector<Card>> hands(6);
    hands[3] = {
        makeCard(0, Suit::HEART, Rank::R2),
        makeCard(0, Suit::SPADE, Rank::R5),
    };

    GameState state = makeStateWithHands(4, ranks, hands);
    PreviousRoundInfo previous = previousFromState(state, 0);
    RuleContext ctx;
    ctx.currentLevel = 2;

    auto result = manager.resolveRound(state, config, previous, ctx);
    ASSERT(result.tributes.size() == 1);
    ASSERT(state.getCardById(result.tributes[0].cardId).rank == Rank::R5);
}

TEST(test_tribute_six_triple_down) {
    TributeManager manager;
    GameRuleConfig config;
    config.playerCount = 6;
    config.playersPerTeam = 3;
    config.enableTribute = true;

    std::array<int, 6> ranks{1, 2, 3, 4, 5, 6};
    std::vector<std::vector<Card>> hands(6);
    hands[3] = {makeCard(0, Suit::SPADE, Rank::R9)};
    hands[4] = {makeCard(0, Suit::CLUB, Rank::Q)};
    hands[5] = {makeCard(0, Suit::DIAMOND, Rank::A)};
    hands[0] = {makeCard(0, Suit::HEART, Rank::R3)};
    hands[1] = {makeCard(0, Suit::HEART, Rank::R4)};
    hands[2] = {makeCard(0, Suit::HEART, Rank::R5)};

    GameState state = makeStateWithHands(6, ranks, hands);
    PreviousRoundInfo previous = previousFromState(state, 0);
    RuleContext ctx;

    auto result = manager.resolveRound(state, config, previous, ctx);
    ASSERT(result.tributes.size() == 3);
    ASSERT(result.tributes[0].toSeat == 0);
    ASSERT(result.tributes[1].toSeat == 1);
    ASSERT(result.tributes[2].toSeat == 2);
    ASSERT(state.getCardById(result.tributes[0].cardId).rank == Rank::A);
    ASSERT(state.getCardById(result.tributes[1].cardId).rank == Rank::Q);
    ASSERT(state.getCardById(result.tributes[2].cardId).rank == Rank::R9);
}

TEST(test_return_tribute_primary_any_leq_ten_non_level) {
    TributeManager manager;
    RuleContext ctx;
    ctx.currentLevel = 2;

    std::array<int, 6> ranks{};
    std::vector<std::vector<Card>> hands(6);
    hands[0] = {
        makeCard(1, Suit::DIAMOND, Rank::R3),
        makeCard(2, Suit::CLUB, Rank::R8),
        makeCard(3, Suit::HEART, Rank::R10),
        makeCard(4, Suit::SPADE, Rank::K),
    };

    GameState state = makeStateWithHands(4, ranks, hands);
    const auto valid = manager.validReturnCardIds(state.players[0].hand, state, ctx);
    ASSERT(valid.size() == 3);
    ASSERT(manager.isValidReturnSubmission(0, 1, state, ctx));
    ASSERT(manager.isValidReturnSubmission(0, 2, state, ctx));
    ASSERT(manager.isValidReturnSubmission(0, 3, state, ctx));
    ASSERT(!manager.isValidReturnSubmission(0, 4, state, ctx));
}

TEST(test_return_tribute_fallback_smallest_single_rank) {
    TributeManager manager;
    RuleContext ctx;
    ctx.currentLevel = 2;

    std::array<int, 6> ranks{};
    std::vector<std::vector<Card>> hands(6);
    hands[0] = {
        makeCard(1, Suit::SPADE, Rank::R2),
        makeCard(2, Suit::HEART, Rank::R2),
        makeCard(3, Suit::CLUB, Rank::Q),
        makeCard(4, Suit::DIAMOND, Rank::K),
        makeCard(5, Suit::SPADE, Rank::A),
    };

    GameState state = makeStateWithHands(4, ranks, hands);
    const auto valid = manager.validReturnCardIds(state.players[0].hand, state, ctx);
    ASSERT(valid.size() == 1);
    ASSERT(state.getCardById(valid[0]).rank == Rank::Q);
    ASSERT(manager.isValidReturnSubmission(0, 3, state, ctx));
    ASSERT(!manager.isValidReturnSubmission(0, 1, state, ctx));
    ASSERT(manager.pickReturnCard(state.players[0].hand, state, ctx) == 3);
}

TEST(test_return_tribute_fallback_multiple_smallest) {
    TributeManager manager;
    RuleContext ctx;
    ctx.currentLevel = 2;

    std::array<int, 6> ranks{};
    std::vector<std::vector<Card>> hands(6);
    hands[0] = {
        makeCard(1, Suit::SPADE, Rank::R2),
        makeCard(2, Suit::HEART, Rank::R2),
        makeCard(3, Suit::CLUB, Rank::Q),
        makeCard(4, Suit::DIAMOND, Rank::Q),
        makeCard(5, Suit::SPADE, Rank::K),
    };

    GameState state = makeStateWithHands(4, ranks, hands);
    const auto valid = manager.validReturnCardIds(state.players[0].hand, state, ctx);
    ASSERT(valid.size() == 2);
    ASSERT(manager.isValidReturnSubmission(0, 3, state, ctx));
    ASSERT(manager.isValidReturnSubmission(0, 4, state, ctx));
    ASSERT(!manager.isValidReturnSubmission(0, 5, state, ctx));
}

TEST(test_return_tribute_level_card_excluded_from_primary) {
    TributeManager manager;
    RuleContext ctx;
    ctx.currentLevel = 5;

    std::array<int, 6> ranks{};
    std::vector<std::vector<Card>> hands(6);
    hands[0] = {
        makeCard(1, Suit::SPADE, Rank::R5),
        makeCard(2, Suit::HEART, Rank::R5),
        makeCard(3, Suit::CLUB, Rank::R6),
        makeCard(4, Suit::DIAMOND, Rank::Q),
    };

    GameState state = makeStateWithHands(4, ranks, hands);
    const auto valid = manager.validReturnCardIds(state.players[0].hand, state, ctx);
    ASSERT(valid.size() == 1);
    ASSERT(state.getCardById(valid[0]).rank == Rank::R6);
    ASSERT(!manager.isValidReturnSubmission(0, 1, state, ctx));
    ASSERT(manager.isValidReturnSubmission(0, 3, state, ctx));
}
