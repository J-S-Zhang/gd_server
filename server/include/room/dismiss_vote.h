#pragma once

#include "game/types.h"
#include <unordered_map>
#include <vector>

namespace guandan {

struct DismissVoteEntry {
    PlayerId playerId = 0;
    std::string nickname;
    bool voted = false;
    bool agree = false;
};

class DismissVote {
public:
    bool active() const { return active_; }
    PlayerId requesterId() const { return requesterId_; }

    bool start(PlayerId requesterId, const std::vector<DismissVoteEntry>& humans);
    void cancel();
    bool recordVote(PlayerId playerId, bool agree);
    bool allAgreed() const;
    bool hasRejection() const;
    const std::vector<DismissVoteEntry>& entries() const { return entries_; }

private:
    bool active_ = false;
    PlayerId requesterId_ = 0;
    std::vector<DismissVoteEntry> entries_;
};

}  // namespace guandan
