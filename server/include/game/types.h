#pragma once

#include <cstdint>
#include <string>

namespace guandan {

using PlayerId = uint64_t;
using RoomId = std::string;
using CardId = uint16_t;
using RequestId = uint64_t;

constexpr int kPlayerCount = 6;
constexpr int kDeckCount = 3;
constexpr int kTotalCards = 162;  // 3 * 54
constexpr int kCardsPerPlayer = 27;
constexpr int kSoloTestCardsPerPlayer = 2;
constexpr int kTeamCount = 2;
constexpr int kPlayersPerTeam = 3;

enum class Suit : uint8_t {
    SPADE = 0,
    HEART = 1,
    CLUB = 2,
    DIAMOND = 3,
    JOKER = 4
};

enum class Rank : uint8_t {
    R2 = 2, R3, R4, R5, R6, R7, R8, R9, R10,
    J = 11, Q, K, A,
    SMALL_JOKER = 15,
    BIG_JOKER = 16
};

enum class CardType {
    INVALID,
    SINGLE,
    PAIR,
    TRIPLE,
    TRIPLE_WITH_PAIR,
    STRAIGHT,
    THREE_PAIRS,
    TWO_TRIPLES,
    STRAIGHT_FLUSH,
    BOMB,
    JOKER_BOMB
};

enum class GamePhase {
    WAITING,
    READY,
    DEALING,
    PLAYING,
    ROUND_END,
    SETTLEMENT,
    FINISHED
};

enum class PlayerStatus {
    ONLINE,
    OFFLINE
};

enum class ErrorCode : int {
    OK = 0,
    INVALID_TOKEN = 1001,
    UNAUTHORIZED = 1002,
    ROOM_NOT_FOUND = 2001,
    ROOM_FULL = 2002,
    ALREADY_IN_ROOM = 2003,
    NOT_IN_ROOM = 2004,
    NOT_ROOM_OWNER = 2005,
    NOT_YOUR_TURN = 3001,
    INVALID_CARDS = 3002,
    INVALID_PATTERN = 3003,
    CANNOT_BEAT = 3004,
    INVALID_STATE = 3005,
    REQUEST_EXPIRED = 3006,
    DUPLICATE_REQUEST = 3007,
    NOT_ALL_READY = 3008,
    INVALID_MESSAGE = 4001,
    UNKNOWN_TYPE = 4002,
    INTERNAL_ERROR = 5000
};

inline int rankValue(Rank r) {
    return static_cast<int>(r);
}

inline bool isJoker(Rank r) {
    return r == Rank::SMALL_JOKER || r == Rank::BIG_JOKER;
}

inline std::string suitToString(Suit s) {
    switch (s) {
        case Suit::SPADE: return "S";
        case Suit::HEART: return "H";
        case Suit::CLUB: return "C";
        case Suit::DIAMOND: return "D";
        default: return "J";
    }
}

inline std::string rankToString(Rank r) {
    switch (r) {
        case Rank::R2: return "2";
        case Rank::R3: return "3";
        case Rank::R4: return "4";
        case Rank::R5: return "5";
        case Rank::R6: return "6";
        case Rank::R7: return "7";
        case Rank::R8: return "8";
        case Rank::R9: return "9";
        case Rank::R10: return "10";
        case Rank::J: return "J";
        case Rank::Q: return "Q";
        case Rank::K: return "K";
        case Rank::A: return "A";
        case Rank::SMALL_JOKER: return "SJ";
        case Rank::BIG_JOKER: return "BJ";
        default: return "?";
    }
}

}  // namespace guandan
