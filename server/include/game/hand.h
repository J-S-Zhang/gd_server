#pragma once

#include "game/types.h"
#include <vector>

namespace guandan {

/// 玩家手牌集合（牌 id 列表）
class Hand {
public:
    /// 加入一张牌
    void add(CardId card);
    /// 批量加入
    void addAll(const std::vector<CardId>& cards);
    /// 移除指定牌（必须全部存在）
    bool remove(const std::vector<CardId>& cards);
    /// 是否包含给定牌集合
    bool contains(const std::vector<CardId>& cards) const;
    /// 是否为空
    bool empty() const { return cards_.empty(); }
    /// 张数
    size_t size() const { return cards_.size(); }
    /// 只读牌列表
    const std::vector<CardId>& cards() const { return cards_; }

private:
    std::vector<CardId> cards_;  ///< 手牌 id 列表
};

}  // namespace guandan
