#include "room/room.h"
#include <algorithm>

namespace guandan {

Room::Room(RoomId roomId, PlayerId ownerId, RoomConfig config)
    : roomId_(std::move(roomId)),
      ownerId_(ownerId),
      config_(std::move(config)),
      engine_(toGameRuleConfig(config_)) {}

bool Room::isSeatTaken(int seatIndex, PlayerId exceptId) const {
    for (const auto& p : players_) {
        if (p.seatIndex == seatIndex && p.id != exceptId) {
            return true;
        }
    }
    return false;
}

int Room::firstEmptySeat() const {
    for (int seat = 0; seat < config_.maxPlayers; ++seat) {
        if (!isSeatTaken(seat)) {
            return seat;
        }
    }
    return -1;
}

bool Room::addBotAtSeat(int seatIndex, int botIndex) {
    if (seatIndex < 0 || seatIndex >= config_.maxPlayers) return false;
    if (isSeatTaken(seatIndex)) return false;

    static const char* kBotNames[] = {"机器人A", "机器人B", "机器人C", "机器人D", "机器人E"};
    RoomPlayer rp;
    rp.id = kBotIdBase + static_cast<PlayerId>(botIndex + 1);
    rp.nickname = kBotNames[botIndex % 5];
    rp.seatIndex = seatIndex;
    rp.isReady = true;
    rp.isOwner = false;
    rp.isBot = true;
    players_.push_back(rp);
    return true;
}

void Room::fillBots() {
    int botIndex = 0;
    while (!isFull()) {
        const int seat = firstEmptySeat();
        if (seat < 0) break;
        if (!addBotAtSeat(seat, botIndex)) break;
        ++botIndex;
    }
}

bool Room::join(PlayerId playerId, const std::string& nickname) {
    if (isBotPlayer(playerId)) return false;
    if (isFull()) return false;
    for (const auto& p : players_) {
        if (p.id == playerId) return false;
    }
    const int seat = firstEmptySeat();
    if (seat < 0) return false;

    RoomPlayer rp;
    rp.id = playerId;
    rp.nickname = nickname;
    rp.seatIndex = seat;
    rp.isOwner = (playerId == ownerId_);
    players_.push_back(rp);
    return true;
}

void Room::leave(PlayerId playerId) {
    if (isBotPlayer(playerId)) return;
    players_.erase(
        std::remove_if(players_.begin(), players_.end(),
                       [playerId](const RoomPlayer& p) { return p.id == playerId; }),
        players_.end());
}

bool Room::ready(PlayerId playerId) {
    if (phase_ != RoomPhase::WAITING) return false;
    for (auto& p : players_) {
        if (p.id == playerId) {
            p.isReady = true;
            if (!p.isBot) {
                engine_.setPlayerReady(playerId);
            }
            return true;
        }
    }
    return false;
}

bool Room::unready(PlayerId playerId) {
    if (phase_ != RoomPhase::WAITING) return false;
    if (isBotPlayer(playerId)) return false;
    for (auto& p : players_) {
        if (p.id == playerId) {
            p.isReady = false;
            return true;
        }
    }
    return false;
}

bool Room::changeSeat(PlayerId playerId, int seatIndex) {
    if (phase_ != RoomPhase::WAITING) return false;
    if (isBotPlayer(playerId)) return false;
    if (seatIndex < 0 || seatIndex >= config_.maxPlayers) return false;
    if (isSeatTaken(seatIndex, playerId)) return false;

    for (auto& p : players_) {
        if (p.id == playerId) {
            p.seatIndex = seatIndex;
            p.isReady = false;
            return true;
        }
    }
    return false;
}

std::vector<DismissVoteEntry> Room::humanVoteEntries() const {
    std::vector<DismissVoteEntry> entries;
    for (const auto& p : players_) {
        if (p.isBot) continue;
        entries.push_back(DismissVoteEntry{p.id, p.nickname, false, false});
    }
    return entries;
}

bool Room::requestDismiss(PlayerId playerId) {
    if (isBotPlayer(playerId)) return false;
    if (dismissVote_.active()) return false;
    auto humans = humanVoteEntries();
    if (humans.empty()) return false;
    bool found = false;
    for (const auto& h : humans) {
        if (h.playerId == playerId) {
            found = true;
            break;
        }
    }
    if (!found) return false;
    return dismissVote_.start(playerId, humans);
}

bool Room::voteDismiss(PlayerId playerId, bool agree) {
    if (isBotPlayer(playerId)) return false;
    return dismissVote_.recordVote(playerId, agree);
}

void Room::cancelDismissVote() {
    dismissVote_.cancel();
}

bool Room::startGame() {
    if (!isFull()) return false;
    for (const auto& p : players_) {
        if (!p.isReady) return false;
    }

    std::vector<PlayerId> ids(config_.maxPlayers, 0);
    for (const auto& p : players_) {
        if (p.seatIndex < 0 || p.seatIndex >= config_.maxPlayers) return false;
        ids[p.seatIndex] = p.id;
    }
    for (PlayerId id : ids) {
        if (id == 0) return false;
    }

    engine_.init(roomId_, ids);
    for (const auto& p : players_) {
        engine_.setPlayerReady(p.id);
    }
    auto result = engine_.startGame();
    if (result.code == ErrorCode::OK) {
        phase_ = RoomPhase::PLAYING;
        return true;
    }
    return false;
}

}  // namespace guandan
