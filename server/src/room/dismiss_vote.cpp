#include "room/dismiss_vote.h"

namespace guandan {

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

void DismissVote::cancel() {
    active_ = false;
    requesterId_ = 0;
    entries_.clear();
}

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

bool DismissVote::allAgreed() const {
    if (!active_ || entries_.empty()) return false;
    for (const auto& entry : entries_) {
        if (!entry.voted || !entry.agree) return false;
    }
    return true;
}

bool DismissVote::hasRejection() const {
    for (const auto& entry : entries_) {
        if (entry.voted && !entry.agree) return true;
    }
    return false;
}

}  // namespace guandan
