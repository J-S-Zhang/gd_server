#include "room/dismiss_vote.h"

/// @file dismiss_vote.cpp
/// @brief 房间解散投票状态机

namespace guandan {

/// 发起解散投票
bool DismissVote::start(PlayerId requesterId, const std::vector<DismissVoteEntry>& humans) {
    if (humans.empty()) return false;
    active_ = true;
    requesterId_ = requesterId;
    entries_ = humans;
    for (auto& entry : entries_) {
        if (entry.playerId == requesterId) {
            entry.voted = true;
            entry.agree = true;
        }
    }
    return true;
}

/// 取消解散投票
void DismissVote::cancel() {
    active_ = false;
    requesterId_ = 0;
    entries_.clear();
}

/// 记录玩家投票
bool DismissVote::recordVote(PlayerId playerId, bool agree) {
    if (!active_) return false;
    for (auto& entry : entries_) {
        if (entry.playerId == playerId) {
            entry.voted = true;
            entry.agree = agree;
            return true;
        }
    }
    return false;
}

/// 判断是否全员同意解散
bool DismissVote::allAgreed() const {
    if (!active_ || entries_.empty()) return false;
    for (const auto& entry : entries_) {
        if (!entry.voted || !entry.agree) return false;
    }
    return true;
}

/// 实现 hasRejection 相关逻辑
bool DismissVote::hasRejection() const {
    for (const auto& entry : entries_) {
        if (entry.voted && !entry.agree) return true;
    }
    return false;
}

}  // namespace guandan
