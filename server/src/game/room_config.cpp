#include "game/room_config.h"

namespace guandan {

RoomConfig RoomConfig::fromModeName(const std::string& mode) {
    RoomConfig config;
    if (mode == "four" || mode == "4") {
        config.mode = GameMode::FOUR;
        config.modeName = "four";
        config.maxPlayers = 4;
        config.deckCount = 2;
        config.playersPerTeam = 2;
    } else if (mode == "solo" || mode == "single") {
        config.mode = GameMode::SOLO;
        config.modeName = "solo";
        config.maxPlayers = 4;
        config.deckCount = 2;
        config.playersPerTeam = 2;
    } else {
        config.mode = GameMode::SIX;
        config.modeName = "six";
        config.maxPlayers = 6;
        config.deckCount = 3;
        config.playersPerTeam = 3;
    }
    return config;
}

}  // namespace guandan
