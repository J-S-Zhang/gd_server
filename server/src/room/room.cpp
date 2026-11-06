#include "room/room.h"
#include "room/room_executor.h"
#include <algorithm>

/// @file room.cpp
/// @brief 房间座位、准备、解散投票与对局状态

namespace guandan {

/// 构造 Room 实例
Room::Room(RoomId roomId, PlayerId ownerId, RoomConfig config, RoomExecutor& executor)
    : roomId_(std::move(roomId)),
      ownerId_(ownerId),
      config_(std::move(config)),
      engine_(toGameRuleConfig(config_)),
      taskQueue_(std::make_shared<RoomTaskQueue>(executor)) {}

/// 实现 isSeatTaken 相关逻辑
bool Room::isSeatTaken(int seatIndex, PlayerId exceptId) const {
    for (const auto& p : players_) {
        if (p.seatIndex == seatIndex && p.id != exceptId) {
            return true;
        }
    }
    return false;
}

/// 实现 firstEmptySeat 相关逻辑
int Room::firstEmptySeat() const {
    for (int seat = 0; seat < config_.maxPlayers; ++seat) {
        if (!isSeatTaken(seat)) {
            return seat;
        }
    }
    return -1;
}

/// 实现 addBotAtSeat 相关逻辑
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

/// 实现 fillBots 相关逻辑
void Room::fillBots() {
    int botIndex = 0;
    while (!isFull()) {
        const int seat = firstEmptySeat();
        if (seat < 0) break;
        if (!addBotAtSeat(seat, botIndex)) break;
        ++botIndex;
    }
}

/// 加入房间或会话
bool Room::join(PlayerId playerId, const PlayerJoinInfo& info) {
    if (isBotPlayer(playerId)) return false;
    if (isFull()) return false;
    for (const auto& p : players_) {
        if (p.id == playerId) return false;
    }
    const int seat = firstEmptySeat();
    if (seat < 0) return false;

    RoomPlayer rp;
    rp.id = playerId;
    rp.nickname = info.nickname;
    rp.avatar = info.avatar;
    rp.avatarPreset = info.avatarPreset;
    rp.seatIndex = seat;
    rp.isOwner = (playerId == ownerId_);
    players_.push_back(rp);
    return true;
}

/// 实现 syncPlayerProfile 相关逻辑
bool Room::syncPlayerProfile(PlayerId playerId, const PlayerJoinInfo& info) {
    if (isBotPlayer(playerId)) return false;
    for (auto& p : players_) {
        if (p.id != playerId || p.isBot) continue;
        if (!info.nickname.empty()) {
            p.nickname = info.nickname;
        }
        p.avatar = info.avatar;
        p.avatarPreset = info.avatarPreset;
        return true;
    }
    return false;
}

/// 离开房间或会话
void Room::leave(PlayerId playerId) {
    if (isBotPlayer(playerId)) return;
    players_.erase(
        std::remove_if(players_.begin(), players_.end(),
                       [playerId](const RoomPlayer& p) { return p.id == playerId; }),
        players_.end());
}

/// 标记玩家已准备
bool Room::ready(PlayerId playerId) {
    if (phase_ != RoomPhase::WAITING && phase_ != RoomPhase::SETTLEMENT) return false;
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

/// 取消准备状态
bool Room::unready(PlayerId playerId) {
    if (phase_ != RoomPhase::WAITING && phase_ != RoomPhase::SETTLEMENT) return false;
    if (isBotPlayer(playerId)) return false;
    for (auto& p : players_) {
        if (p.id == playerId) {
            p.isReady = false;
            return true;
        }
    }
    return false;
}

/// 实现 changeSeat 相关逻辑
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

/// 实现 humanVoteEntries 相关逻辑
std::vector<DismissVoteEntry> Room::humanVoteEntries() const {
    std::vector<DismissVoteEntry> entries;
    for (const auto& p : players_) {
        if (p.isBot) continue;
        entries.push_back(DismissVoteEntry{p.id, p.nickname, false, false});
    }
    return entries;
}

/// 实现 requestDismiss 相关逻辑
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

/// 实现 voteDismiss 相关逻辑
bool Room::voteDismiss(PlayerId playerId, bool agree) {
    if (isBotPlayer(playerId)) return false;
    return dismissVote_.recordVote(playerId, agree);
}

/// 实现 cancelDismissVote 相关逻辑
void Room::cancelDismissVote() {
    dismissVote_.cancel();
}

/// 实现 setEnableTribute 相关逻辑
bool Room::setEnableTribute(PlayerId playerId, bool enabled) {
    if (phase_ != RoomPhase::WAITING) return false;
    if (playerId != ownerId_) return false;
    if (isBotPlayer(playerId)) return false;
    config_.enableTribute = enabled;
    engine_.updateConfig(toGameRuleConfig(config_));
    return true;
}

/// 实现 clearSpectateTargets 相关逻辑
void Room::clearSpectateTargets() {
    spectateTargets_.clear();
}

/// 实现 spectateTargetFor 相关逻辑
int Room::spectateTargetFor(PlayerId viewerId) const {
    const auto it = spectateTargets_.find(viewerId);
    if (it == spectateTargets_.end()) return -1;
    return it->second;
}

/// 实现 spectatableTeammateSeats 相关逻辑
std::vector<int> Room::spectatableTeammateSeats(PlayerId viewerId) const {
    const auto& state = engine_.getState();
    int viewerSeat = -1;
    for (int i = 0; i < state.playerCount; ++i) {
        if (state.players[i].id == viewerId) {
            viewerSeat = i;
            break;
        }
    }
    if (viewerSeat < 0 || !state.players[viewerSeat].hasFinished) {
        return {};
    }

    const int team = state.players[viewerSeat].team;
    std::vector<int> seats;
    for (int i = 0; i < state.playerCount; ++i) {
        if (i == viewerSeat) continue;
        if (state.players[i].team == team && !state.players[i].hasFinished) {
            seats.push_back(i);
        }
    }
    return seats;
}

/// 实现 resolveViewAnchor 相关逻辑
int Room::resolveViewAnchor(PlayerId viewerId) const {
    const auto& state = engine_.getState();
    int viewerSeat = -1;
    for (int i = 0; i < state.playerCount; ++i) {
        if (state.players[i].id == viewerId) {
            viewerSeat = i;
            break;
        }
    }
    if (viewerSeat < 0) return -1;
    if (!state.players[viewerSeat].hasFinished) return viewerSeat;

    const auto teammates = spectatableTeammateSeats(viewerId);
    if (teammates.empty()) return viewerSeat;

    const int target = spectateTargetFor(viewerId);
    if (target >= 0 &&
        std::find(teammates.begin(), teammates.end(), target) != teammates.end()) {
        return target;
    }
    return teammates.front();
}

/// 实现 setSpectateTarget 相关逻辑
bool Room::setSpectateTarget(PlayerId viewerId, int targetSeat) {
    const auto teammates = spectatableTeammateSeats(viewerId);
    if (teammates.empty()) return false;
    if (std::find(teammates.begin(), teammates.end(), targetSeat) == teammates.end()) {
        return false;
    }
    spectateTargets_[viewerId] = targetSeat;
    return true;
}

/// 实现 enterSettlement 相关逻辑
void Room::enterSettlement() {
    phase_ = RoomPhase::SETTLEMENT;
    for (auto& p : players_) {
        p.isReady = p.isBot;
        engine_.setPlayerReady(p.id, p.isReady);
    }
}

/// 实现 allReady 相关逻辑
bool Room::allReady() const {
    if (!isFull()) return false;
    for (const auto& p : players_) {
        if (!p.isReady) return false;
    }
    return true;
}

/// 实现 startNextRound 相关逻辑
bool Room::startNextRound() {
    if (phase_ != RoomPhase::SETTLEMENT) return false;
    if (!allReady()) return false;
    auto result = engine_.startNextRound();
    if (result.code == ErrorCode::OK) {
        clearSpectateTargets();
        phase_ = RoomPhase::PLAYING;
        return true;
    }
    return false;
}

/// 实现 finishMatch 相关逻辑
void Room::finishMatch() {
    phase_ = RoomPhase::FINISHED;
}

/// 开始对局
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
        clearSpectateTargets();
        phase_ = RoomPhase::PLAYING;
        return true;
    }
    return false;
}

/// 实现 containsPlayer 相关逻辑
bool Room::containsPlayer(PlayerId playerId) const {
    for (const auto& p : players_) {
        if (p.id == playerId) return true;
    }
    return false;
}

/// 实现 nextEmotionEventId 相关逻辑
uint64_t Room::nextEmotionEventId() {
    return ++emotionEventId_;
}

}  // namespace guandan
