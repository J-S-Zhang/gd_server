#include "protocol/message.h"
#include "test_framework.h"

using namespace guandan;

TEST(test_parse_cards_array) {
    auto cards = parseCardsArray(R"({"cards":[10,23,41]})");
    ASSERT(cards.size() == 3);
    ASSERT(cards[0] == 10);
    ASSERT(cards[1] == 23);
    ASSERT(cards[2] == 41);
}

TEST(test_parse_message_with_data) {
    auto msg = parseMessage(
        R"({"type":"play_cards","request_id":4,"room_id":"938271","turn_id":37,"data":{"cards":[1,2,3]}})"
    );
    ASSERT(msg.type == "play_cards");
    ASSERT(msg.roomId == "938271");
    ASSERT(msg.turnId == 37);
    ASSERT(msg.cards.size() == 3);
}
