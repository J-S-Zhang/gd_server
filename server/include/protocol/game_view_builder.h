#pragma once

#include "game/game_engine.h"
#include "room/room.h"
#include <string>
#include <vector>

namespace guandan {

std::string buildGameSnapshotJson(const PlayerView& view, const RoomId& roomId);
std::string buildPlayerPlayedJson(
    PlayerId playerId,
    const std::vector<CardId>& cards,
    int remainingCardCount,
    bool hasFinished,
    int finishRank,
    int nextPlayerIndex,
    uint64_t stateVersion,
    uint64_t turnId
);
std::string buildPlayerPassedJson(
    int nextPlayerIndex,
    uint64_t stateVersion,
    uint64_t turnId
);
std::string buildRoomStateJson(const Room& room);

}  // namespace guandan
