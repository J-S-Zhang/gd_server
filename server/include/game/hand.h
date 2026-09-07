#pragma once

#include "game/types.h"
#include <vector>

namespace guandan {

class Hand {
public:
    void add(CardId card);
    void addAll(const std::vector<CardId>& cards);
    bool remove(const std::vector<CardId>& cards);
    bool contains(const std::vector<CardId>& cards) const;
    bool empty() const { return cards_.empty(); }
    size_t size() const { return cards_.size(); }
    const std::vector<CardId>& cards() const { return cards_; }

private:
    std::vector<CardId> cards_;
};

}  // namespace guandan
