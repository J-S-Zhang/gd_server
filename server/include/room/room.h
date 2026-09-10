#pragma once

#include "game/game_engine.h"
#include "game/room_config.h"
#include "room/dismiss_vote.h"
#include "room/room_task_queue.h"
#include "game/types.h"
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
    bool isBot = false;
    PlayerStatus status = PlayerStatus::ONLINE;
};

class Room {
public:
    Room(RoomId roomId, PlayerId ownerId, RoomConfig config);

    RoomId id() const { return roomId_; }
    RoomPhase phase() const { return phase_; }
    const RoomConfig& config() const { return config_; }
    const std::vector<RoomPlayer>& players() const { return players_; }
    GameEngine& engine() { return engine_; }
    RoomTaskQueue& taskQueue() { return taskQueue_; }

    bool join(PlayerId playerId, const std::string& nickname);
    void leave(PlayerId playerId);
    bool ready(PlayerId playerId);
    bool unready(PlayerId playerId);
    bool changeSeat(PlayerId playerId, int seatIndex);
    bool setEnableTribute(PlayerId playerId, bool enabled);
    bool startGame();
    bool startNextRound();
    void finishMatch();
    void fillBots();
    bool isFull() const { return static_cast<int>(players_.size()) >= config_.maxPlayers; }
    bool isEmpty() const { return players_.empty(); }
    PlayerId ownerId() const { return ownerId_; }

    bool requestDismiss(PlayerId playerId);
    bool voteDismiss(PlayerId playerId, bool agree);
    void cancelDismissVote();
    const DismissVote& dismissVote() const { return dismissVote_; }
    std::vector<DismissVoteEntry> humanVoteEntries() const;

    int firstEmptySeat() const;
    bool isSeatTaken(int seatIndex, PlayerId exceptId = 0) const;

    using BroadcastFn = std::function<void(PlayerId, const std::string&)>;
    void setBroadcastCallback(BroadcastFn fn) { broadcast_ = std::move(fn); }

private:
    bool addBotAtSeat(int seatIndex, int botIndex);

    RoomId roomId_;
    PlayerId ownerId_;
    RoomConfig config_;
    RoomPhase phase_ = RoomPhase::WAITING;
    std::vector<RoomPlayer> players_;
    GameEngine engine_;
    RoomTaskQueue taskQueue_;
    BroadcastFn broadcast_;
    DismissVote dismissVote_;
};

}  // namespace guandan
