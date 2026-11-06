#pragma once

#include "game/game_engine.h"
#include "game/tribute.h"
#include "room/dismiss_vote.h"
#include "room/room.h"
#include <string>
#include <vector>

namespace guandan {

/// 构建 GAME_SNAPSHOT 的 dataJson
std::string buildGameSnapshotJson(const PlayerView& view, const Room& room);
/// 构建 PLAYER_PLAYED 的 dataJson
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
/// 构建 PLAYER_PASSED 的 dataJson
std::string buildPlayerPassedJson(
    PlayerId playerId,
    int playerSeatIndex,
    int nextPlayerIndex,
    bool roundReset,
    uint64_t stateVersion,
    uint64_t turnId
);
/// 构建 ROOM_STATE 的 dataJson
std::string buildRoomStateJson(const Room& room);
/// 构建解散投票状态的 dataJson
std::string buildDismissVoteJson(const DismissVote& vote);
/// 构建进贡阶段结束结果的 dataJson
std::string buildTributeResolvedJson(const TributeRoundResult& result);

}  // namespace guandan
