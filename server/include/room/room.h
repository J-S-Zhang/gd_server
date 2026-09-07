#pragma once

#include "game/game_engine.h"
#include "room/room_task_queue.h"
#include "game/types.h"
#include <array>
#include <functional>
#include <string>
#include <vector>

namespace guandan {

enum class RoomPhase {
    CREATED,
    WAITING,
    PLAYING,
    SETTLEMENT,
    FINISHED
};

struct RoomPlayer {
    PlayerId id = 0;
    std::string nickname;
    int seatIndex = -1;
    bool isReady = false;
    bool isOwner = false;
    PlayerStatus status = PlayerStatus::ONLINE;
};

class Room {
public:
    Room(RoomId roomId, PlayerId ownerId);

    RoomId id() const { return roomId_; }
    RoomPhase phase() const { return phase_; }
    const std::vector<RoomPlayer>& players() const { return players_; }
    GameEngine& engine() { return engine_; }
    RoomTaskQueue& taskQueue() { return taskQueue_; }

    bool join(PlayerId playerId, const std::string& nickname);
    void leave(PlayerId playerId);
    bool ready(PlayerId playerId);
    bool startGame();
    bool isFull() const { return players_.size() >= kPlayerCount; }
    bool isEmpty() const { return players_.empty(); }
    PlayerId ownerId() const { return ownerId_; }

    using BroadcastFn = std::function<void(PlayerId, const std::string&)>;
    void setBroadcastCallback(BroadcastFn fn) { broadcast_ = std::move(fn); }

private:
    RoomId roomId_;
    PlayerId ownerId_;
    RoomPhase phase_ = RoomPhase::WAITING;
    std::vector<RoomPlayer> players_;
    GameEngine engine_;
    RoomTaskQueue taskQueue_;
    BroadcastFn broadcast_;
};

}  // namespace guandan
