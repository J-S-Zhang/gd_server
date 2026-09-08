#pragma once

#include "game/card.h"
#include <string>

namespace guandan {

constexpr PlayerId kBotIdBase = 9000000000ULL;

enum class GameMode {
    SIX,
    FOUR,
    SOLO
};

struct RoomConfig {
    GameMode mode = GameMode::SIX;
    int maxPlayers = 6;
    int deckCount = 3;
    int playersPerTeam = 3;
    std::string modeName = "six";

    static RoomConfig fromModeName(const std::string& mode);
};

inline bool isBotPlayer(PlayerId id) {
    return id >= kBotIdBase;
}

inline GameRuleConfig toGameRuleConfig(const RoomConfig& config) {
    GameRuleConfig cfg;
    cfg.playerCount = config.maxPlayers;
    cfg.deckCount = config.deckCount;
    cfg.playersPerTeam = config.playersPerTeam;
    switch (config.mode) {
        case GameMode::FOUR:
            cfg.ruleVersion = "four_player_guandan_v1";
            break;
        case GameMode::SOLO:
            cfg.ruleVersion = "solo_test_v1";
            break;
        default:
            cfg.ruleVersion = "six_player_guandan_v1";
            break;
    }
    return cfg;
}

}  // namespace guandan
