#include "game/hand.h"
#include <algorithm>

namespace guandan {

void Hand::add(CardId card) {
    cards_.push_back(card);
}

void Hand::addAll(const std::vector<CardId>& cards) {
    cards_.insert(cards_.end(), cards.begin(), cards.end());
}

bool Hand::remove(const std::vector<CardId>& toRemove) {
    auto temp = cards_;
    for (CardId id : toRemove) {
        auto it = std::find(temp.begin(), temp.end(), id);
        if (it == temp.end()) return false;
        temp.erase(it);
    }
    cards_ = std::move(temp);
    return true;
}

bool Hand::contains(const std::vector<CardId>& query) const {
    for (CardId id : query) {
        if (std::find(cards_.begin(), cards_.end(), id) == cards_.end()) {
            return false;
        }
    }
    return true;
}

}  // namespace guandan
