#pragma once

#include "game/types.h"
#include <unordered_map>
#include <vector>

namespace guandan {

/// 单名玩家对解散投票的记录
struct DismissVoteEntry {
    PlayerId playerId = 0;  ///< 玩家 id
    std::string nickname;     ///< 昵称（展示用）
    bool voted = false;       ///< 是否已投票
    bool agree = false;       ///< 是否同意解散
};

/// 房间解散投票状态机
class DismissVote {
public:
    /// 是否正在进行投票
    bool active() const { return active_; }
    /// 发起投票的玩家 id
    PlayerId requesterId() const { return requesterId_; }

    /// 开始投票，humans 为需参与投票的真人条目
    bool start(PlayerId requesterId, const std::vector<DismissVoteEntry>& humans);
    /// 取消投票
    void cancel();
    /// 记录某玩家投票
    bool recordVote(PlayerId playerId, bool agree);
    /// 是否全员同意
    bool allAgreed() const;
    /// 是否已有拒绝
    bool hasRejection() const;
    /// 所有投票条目
    const std::vector<DismissVoteEntry>& entries() const { return entries_; }

private:
    bool active_ = false;                         ///< 投票是否进行中
    PlayerId requesterId_ = 0;                  ///< 发起人
    std::vector<DismissVoteEntry> entries_;     ///< 各玩家投票状态
};

}  // namespace guandan
