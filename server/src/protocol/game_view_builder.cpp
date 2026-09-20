#include "protocol/game_view_builder.h"
#include "room/room.h"
#include <sstream>

namespace guandan {

static std::string phaseToString(GamePhase phase) {
    switch (phase) {
        case GamePhase::WAITING: return "WAITING";
        case GamePhase::READY: return "READY";
        case GamePhase::DEALING: return "DEALING";
        case GamePhase::TRIBUTE: return "TRIBUTE";
        case GamePhase::RETURN_TRIBUTE: return "RETURN_TRIBUTE";
        case GamePhase::PLAYING: return "PLAYING";
        case GamePhase::ROUND_END: return "ROUND_END";
        case GamePhase::SETTLEMENT: return "SETTLEMENT";
        case GamePhase::FINISHED: return "FINISHED";
        default: return "WAITING";
    }
}

static std::string escapeJson(const std::string& s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

static const RoomPlayer* findRoomPlayer(const Room& room, PlayerId id) {
    for (const auto& p : room.players()) {
        if (p.id == id) return &p;
    }
    return nullptr;
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

std::string buildGameSnapshotJson(const PlayerView& view, const Room& room) {
    const RoomId& roomId = room.id();
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
    oss << ",\"first_player_index\":" << view.firstPlayerIndex;
    oss << ",\"my_seat_index\":" << view.mySeatIndex;
    oss << ",\"viewer_seat_index\":" << view.viewerSeatIndex;
    oss << ",\"is_spectating\":" << (view.isSpectating ? "true" : "false");
    oss << ",\"spectatable_teammates\":[";
    for (size_t i = 0; i < view.spectatableTeammates.size(); ++i) {
        if (i > 0) oss << ",";
        oss << view.spectatableTeammates[i];
    }
    oss << "]";
    oss << ",\"my_cards\":" << cardIdsToJsonArray(view.myCards);
    oss << ",\"last_played_cards\":" << cardIdsToJsonArray(view.lastPlayedCards);
    oss << ",\"last_played_player_index\":" << view.lastPlayedPlayerIndex;
    oss << ",\"pending_tributer_seats\":[";
    for (size_t i = 0; i < view.pendingTributerSeats.size(); ++i) {
        if (i > 0) oss << ",";
        oss << view.pendingTributerSeats[i];
    }
    oss << "]";
    oss << ",\"pending_return_seats\":[";
    for (size_t i = 0; i < view.pendingReturnSeats.size(); ++i) {
        if (i > 0) oss << ",";
        oss << view.pendingReturnSeats[i];
    }
    oss << "]";
    oss << ",\"required_tribute_card_id\":" << view.requiredTributeCardId;
    oss << ",\"must_return_tribute\":" << (view.mustReturnTribute ? "true" : "false");
    oss << ",\"valid_return_card_ids\":" << cardIdsToJsonArray(view.validReturnCardIds);
    oss << ",\"tribute_seat_plays\":[";
    for (size_t i = 0; i < view.tributeSeatPlays.size(); ++i) {
        const auto& play = view.tributeSeatPlays[i];
        if (i > 0) oss << ",";
        oss << "{\"seat_index\":" << play.seatIndex;
        oss << ",\"card_id\":" << play.cardId;
        oss << ",\"kind\":\"" << escapeJson(play.kind) << "\"}";
    }
    oss << "]";

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
        const RoomPlayer* rp = findRoomPlayer(room, p.id);
        std::string nickname = rp && !rp->nickname.empty()
                                   ? rp->nickname
                                   : ("Player" + std::to_string(p.id));
        oss << ",\"nickname\":\"" << escapeJson(nickname) << "\"";
        if (rp && !rp->avatar.empty()) {
            oss << ",\"avatar\":\"" << escapeJson(rp->avatar) << "\"";
        }
        if (rp && !rp->avatarPreset.empty()) {
            oss << ",\"avatar_preset\":\"" << escapeJson(rp->avatarPreset) << "\"";
        }
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

static const char* roomPhaseToString(RoomPhase phase) {
    switch (phase) {
        case RoomPhase::CREATED: return "CREATED";
        case RoomPhase::WAITING: return "WAITING";
        case RoomPhase::PLAYING: return "PLAYING";
        case RoomPhase::SETTLEMENT: return "SETTLEMENT";
        case RoomPhase::FINISHED: return "FINISHED";
    }
    return "WAITING";
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
        oss << ",\"nickname\":\"" << escapeJson(p.nickname) << "\"";
        if (!p.avatar.empty()) {
            oss << ",\"avatar\":\"" << escapeJson(p.avatar) << "\"";
        }
        if (!p.avatarPreset.empty()) {
            oss << ",\"avatar_preset\":\"" << escapeJson(p.avatarPreset) << "\"";
        }
        oss << ",\"is_ready\":" << (p.isReady ? "true" : "false");
        oss << ",\"is_owner\":" << (p.isOwner ? "true" : "false");
        oss << ",\"is_bot\":" << (p.isBot ? "true" : "false");
        oss << ",\"status\":\"" << (p.status == PlayerStatus::ONLINE ? "online" : "offline") << "\"";
        oss << "}";
    }
    oss << "],\"player_count\":" << players.size();
    oss << ",\"max_players\":" << config.maxPlayers;
    oss << ",\"mode\":\"" << config.modeName << "\"";
    oss << ",\"enable_tribute\":" << (config.enableTribute ? "true" : "false");
    oss << ",\"room_phase\":\"" << roomPhaseToString(room.phase()) << "\"";
    oss << "}";
    return oss.str();
}

static std::string tributeTransfersToJson(const std::vector<TributeTransfer>& transfers) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < transfers.size(); ++i) {
        if (i > 0) oss << ",";
        const auto& tr = transfers[i];
        oss << "{";
        oss << "\"from_seat\":" << tr.fromSeat;
        oss << ",\"to_seat\":" << tr.toSeat;
        oss << ",\"card_id\":" << tr.cardId;
        oss << "}";
    }
    oss << "]";
    return oss.str();
}

std::string buildTributeResolvedJson(const TributeRoundResult& result) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"skipped\":" << (result.skipped ? "true" : "false");
    oss << ",\"anti_tribute\":" << (result.antiTribute ? "true" : "false");
    oss << ",\"first_player_seat\":" << result.firstPlayerSeat;
    oss << ",\"head_tributer_seat\":" << result.headTributerSeat;
    oss << ",\"summary\":\"" << result.summary << "\"";
    oss << ",\"tributes\":" << tributeTransfersToJson(result.tributes);
    oss << ",\"returns\":" << tributeTransfersToJson(result.returns);
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
