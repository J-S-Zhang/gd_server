#include "game/game_engine.h"
#include "test_framework.h"
#include <vector>

using namespace guandan;

TEST(test_init_and_start) {
    GameEngine engine;
    std::vector<PlayerId> players = {1001, 1002, 1003, 1004, 1005, 1006};
    engine.init("123456", players);

    for (auto id : players) engine.setPlayerReady(id);
    ASSERT(engine.allReady());

    auto result = engine.startGame();
    ASSERT(result.code == ErrorCode::OK);
    ASSERT(engine.getState().phase == GamePhase::PLAYING);

    for (const auto& p : engine.getState().players) {
        ASSERT(p.hand.size() == 27);
    }
}

TEST(test_player_view_hides_cards) {
    GameEngine engine;
    std::vector<PlayerId> players = {1001, 1002, 1003, 1004, 1005, 1006};
    engine.init("123456", players);
    for (auto id : players) engine.setPlayerReady(id);
    engine.startGame();

    auto view = engine.buildViewFor(1001);
    ASSERT(view.myCards.size() == 27);
    ASSERT(view.others.size() == 5);
    for (const auto& o : view.others) {
        ASSERT(o.cardCount == 27);
    }
}

TEST(test_not_your_turn) {
    GameEngine engine;
    std::vector<PlayerId> players = {1001, 1002, 1003, 1004, 1005, 1006};
    engine.init("123456", players);
    for (auto id : players) engine.setPlayerReady(id);
    engine.startGame();

    auto cards = engine.getState().players[1].hand.cards();
    if (!cards.empty()) {
        auto result = engine.playCards(1002, {cards[0]});
        ASSERT(result.code == ErrorCode::NOT_YOUR_TURN);
    }
}
