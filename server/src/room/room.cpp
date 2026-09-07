#include "room/room.h"
#include <algorithm>

namespace guandan {

Room::Room(RoomId roomId, PlayerId ownerId)
    : roomId_(std::move(roomId)), ownerId_(ownerId) {}

bool Room::join(PlayerId playerId, const std::string& nickname) {
    if (isFull()) return false;
    for (const auto& p : players_) {
        if (p.id == playerId) return false;
    }
    RoomPlayer rp;
    rp.id = playerId;
    rp.nickname = nickname;
    rp.seatIndex = static_cast<int>(players_.size());
    rp.isOwner = (playerId == ownerId_);
    players_.push_back(rp);
    return true;
}

void Room::leave(PlayerId playerId) {
    players_.erase(
        std::remove_if(players_.begin(), players_.end(),
                       [playerId](const RoomPlayer& p) { return p.id == playerId; }),
        players_.end());
    for (size_t i = 0; i < players_.size(); ++i) {
        players_[i].seatIndex = static_cast<int>(i);
    }
}

bool Room::ready(PlayerId playerId) {
    for (auto& p : players_) {
        if (p.id == playerId) {
            p.isReady = true;
            engine_.setPlayerReady(playerId);
            return true;
        }
    }
    return false;
}

bool Room::startGame() {
    if (!isFull()) return false;
    std::vector<PlayerId> ids;
    for (const auto& p : players_) {
        ids.push_back(p.id);
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
