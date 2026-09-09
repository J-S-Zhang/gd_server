#include "protocol/game_view_builder.h"
#include "room/room.h"
#include <sstream>

namespace guandan {

static std::string phaseToString(GamePhase phase) {
    switch (phase) {
        case GamePhase::WAITING: return "WAITING";
        case GamePhase::READY: return "READY";
        case GamePhase::DEALING: return "DEALING";
        case GamePhase::PLAYING: return "PLAYING";
        case GamePhase::ROUND_END: return "ROUND_END";
        case GamePhase::SETTLEMENT: return "SETTLEMENT";
        case GamePhase::FINISHED: return "FINISHED";
        default: return "WAITING";
    }
}

static std::string cardIdsToJsonArray(const std::vector<CardId>& cards) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < cards.size(); ++i) {
        if (i > 0) oss << ",";
        oss << cards[i];
    }
    oss << "]";
    return oss.str();
}

std::string buildGameSnapshotJson(const PlayerView& view, const RoomId& roomId) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"room_id\":\"" << roomId << "\"";
    oss << ",\"phase\":\"" << phaseToString(view.phase) << "\"";
    oss << ",\"state_version\":" << view.stateVersion;
    oss << ",\"turn_id\":" << view.turnId;
    oss << ",\"current_level\":" << view.currentLevel;
    oss << ",\"attacking_team\":" << view.attackingTeam;
    oss << ",\"is_pass_a_round\":" << (view.isPassARound ? "true" : "false");
    oss << ",\"is_playing_own_round\":" << (view.isPlayingOwnRound ? "true" : "false");
    oss << ",\"team_levels\":[" << view.teamLevels[0] << "," << view.teamLevels[1] << "]";
    oss << ",\"in_pass_a_phase\":["
        << (view.inPassAPhase[0] ? "true" : "false") << ","
        << (view.inPassAPhase[1] ? "true" : "false") << "]";
    oss << ",\"pass_a_fail_counts\":[" << view.passAFailCounts[0] << ","
        << view.passAFailCounts[1] << "]";
    oss << ",\"current_player_index\":" << view.currentPlayerIndex;
    oss << ",\"my_seat_index\":" << view.mySeatIndex;
    oss << ",\"my_cards\":" << cardIdsToJsonArray(view.myCards);
    oss << ",\"last_played_cards\":" << cardIdsToJsonArray(view.lastPlayedCards);
    oss << ",\"last_played_player_index\":" << view.lastPlayedPlayerIndex;

    oss << ",\"players\":[";
    for (size_t i = 0; i < view.others.size(); ++i) {
        const auto& p = view.others[i];
        if (i > 0) oss << ",";
        oss << "{";
        oss << "\"id\":" << p.id;
        oss << ",\"seat_index\":" << p.seatIndex;
        oss << ",\"team\":" << p.team;
        oss << ",\"card_count\":" << p.cardCount;
        oss << ",\"has_finished\":" << (p.hasFinished ? "true" : "false");
        oss << ",\"finish_rank\":" << p.finishRank;
        oss << ",\"is_ready\":" << (p.isReady ? "true" : "false");
        oss << ",\"status\":\"" << (p.status == PlayerStatus::ONLINE ? "online" : "offline") << "\"";
        oss << ",\"nickname\":\"Player" << p.id << "\"";
        oss << "}";
    }
    oss << "]}";
    return oss.str();
}

std::string buildPlayerPlayedJson(
    PlayerId playerId,
    const std::vector<CardId>& cards,
    int remainingCardCount,
    bool hasFinished,
    int finishRank,
    int nextPlayerIndex,
    uint64_t stateVersion,
    uint64_t turnId
) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"player_id\":" << playerId;
    oss << ",\"cards\":" << cardIdsToJsonArray(cards);
    oss << ",\"card_count\":" << remainingCardCount;
    oss << ",\"has_finished\":" << (hasFinished ? "true" : "false");
    oss << ",\"finish_rank\":" << finishRank;
    oss << ",\"next_player\":" << nextPlayerIndex;
    oss << ",\"state_version\":" << stateVersion;
    oss << ",\"turn_id\":" << turnId;
    oss << "}";
    return oss.str();
}

std::string buildPlayerPassedJson(
    PlayerId playerId,
    int playerSeatIndex,
    int nextPlayerIndex,
    bool roundReset,
    uint64_t stateVersion,
    uint64_t turnId
) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"player_id\":" << playerId;
    oss << ",\"seat_index\":" << playerSeatIndex;
    oss << ",\"next_player\":" << nextPlayerIndex;
    oss << ",\"round_reset\":" << (roundReset ? "true" : "false");
    oss << ",\"state_version\":" << stateVersion;
    oss << ",\"turn_id\":" << turnId;
    oss << "}";
    return oss.str();
}

std::string buildRoomStateJson(const Room& room) {
    const auto& players = room.players();
    const auto& config = room.config();
    std::ostringstream oss;
    oss << "{\"players\":[";
    for (size_t i = 0; i < players.size(); ++i) {
        const auto& p = players[i];
        if (i > 0) oss << ",";
        oss << "{";
        oss << "\"id\":" << p.id;
        oss << ",\"seat_index\":" << p.seatIndex;
        oss << ",\"team\":" << (p.seatIndex % 2);
        oss << ",\"nickname\":\"" << p.nickname << "\"";
        oss << ",\"is_ready\":" << (p.isReady ? "true" : "false");
        oss << ",\"is_owner\":" << (p.isOwner ? "true" : "false");
        oss << ",\"is_bot\":" << (p.isBot ? "true" : "false");
        oss << ",\"status\":\"" << (p.status == PlayerStatus::ONLINE ? "online" : "offline") << "\"";
        oss << "}";
    }
    oss << "],\"player_count\":" << players.size();
    oss << ",\"max_players\":" << config.maxPlayers;
    oss << ",\"mode\":\"" << config.modeName << "\"";
    oss << "}";
    return oss.str();
}

std::string buildDismissVoteJson(const DismissVote& vote) {
    std::ostringstream oss;
    oss << "{\"requester_id\":" << vote.requesterId();
    oss << ",\"votes\":[";
    const auto& entries = vote.entries();
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        if (i > 0) oss << ",";
        oss << "{";
        oss << "\"player_id\":" << entry.playerId;
        oss << ",\"nickname\":\"" << entry.nickname << "\"";
        oss << ",\"voted\":" << (entry.voted ? "true" : "false");
        oss << ",\"agree\":" << (entry.agree ? "true" : "false");
        oss << "}";
    }
    oss << "]}";
    return oss.str();
}

}  // namespace guandan
