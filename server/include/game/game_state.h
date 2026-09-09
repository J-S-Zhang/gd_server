#pragma once

#include "game/card.h"
#include "game/hand.h"
#include "game/types.h"
#include "game/card_pattern.h"
#include <array>
#include <optional>
#include <vector>

namespace guandan {

struct PlayerState {
    PlayerId id = 0;
    int seatIndex = 0;
    int team = 0;           // 0 = A, 1 = B
    Hand hand;
    PlayerStatus status = PlayerStatus::ONLINE;
    bool isReady = false;
    bool hasFinished = false;
    int finishRank = 0;     // 0 = not finished, 1 = 头游, etc.
};

struct GameState {
    RoomId roomId;
    GamePhase phase = GamePhase::WAITING;
    std::array<PlayerState, kPlayerCount> players{};
    int playerCount = 0;

    int currentPlayerIndex = 0;
    int currentLevel = 2;
    int attackingTeam = 0;     // 攻方（打自己级牌的队伍）
    bool isPassARound = false; // 本局是否为过 A 局
    int firstPlayerIndex = 0;  // 本局首出

    std::vector<CardId> lastPlayedCards;
    CardPattern lastPattern;
    int lastPlayedPlayerIndex = -1;
    int passCount = 0;

    uint64_t turnId = 0;
    uint64_t stateVersion = 0;
    int round = 0;

    std::vector<Card> allCards;  // id -> card lookup

    const Card& getCardById(CardId id) const;
    PlayerState* getPlayerById(PlayerId id);
    const PlayerState* getPlayerById(PlayerId id) const;
    int teamOf(int seatIndex) const { return seatIndex % 2; }
};

}  // namespace guandan
