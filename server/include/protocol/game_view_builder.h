#pragma once

#include "game/game_engine.h"
#include "game/tribute.h"
#include "room/dismiss_vote.h"
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
    PlayerId playerId,
    int playerSeatIndex,
    int nextPlayerIndex,
    bool roundReset,
    uint64_t stateVersion,
    uint64_t turnId
);
std::string buildRoomStateJson(const Room& room);
std::string buildDismissVoteJson(const DismissVote& vote);
std::string buildTributeResolvedJson(const TributeRoundResult& result);

}  // namespace guandan
