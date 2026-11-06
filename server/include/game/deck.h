#pragma once

#include "game/card.h"
#include <random>
#include <vector>

namespace guandan {

/// 一副或多副牌的容器与发牌
class Deck {
public:
    explicit Deck(int deckCount = kDeckCount);

    /// 随机洗牌
    void shuffle();
    /// 指定种子洗牌（可复现）
    void shuffle(uint64_t seed);

    /// cardsPerPlayer <= 0：整副牌均分；> 0：洗牌后每人随机发固定张数。
    std::vector<std::vector<CardId>> deal(int playerCount, int cardsPerPlayer = 0);

    /// 全部牌对象
    const std::vector<Card>& cards() const { return cards_; }
    /// 按 id 查牌
    Card getCard(CardId id) const;

private:
    std::vector<Card> cards_;  ///< 牌堆
};

}  // namespace guandan
