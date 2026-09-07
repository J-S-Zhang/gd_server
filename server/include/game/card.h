#pragma once

#include "game/types.h"
#include <string>
#include <vector>

namespace guandan {

struct Card {
    CardId id = 0;
    Suit suit = Suit::SPADE;
    Rank rank = Rank::R2;

    bool operator==(const Card& other) const {
        return id == other.id;
    }

    std::string toString() const {
        if (suit == Suit::JOKER) {
            return rankToString(rank);
        }
        return rankToString(rank) + suitToString(suit);
    }
};

struct GameRuleConfig {
    int playerCount = kPlayerCount;
    int deckCount = kDeckCount;
    int teamCount = kTeamCount;
    int playersPerTeam = kPlayersPerTeam;
    bool enableTribute = false;
    bool enableReturnTribute = false;
    bool enableAntiTribute = false;
    bool enableWildCard = true;
    int turnTimeoutSeconds = 30;
    std::string ruleVersion = "six_player_guandan_v1";
};

struct RuleContext {
    int currentLevel = 2;  // 2-14 (2 to A)
    bool enableWildCard = true;

    Rank levelRank() const {
        return static_cast<Rank>(currentLevel);
    }

    bool isWildCard(const Card& card) const {
        if (!enableWildCard) return false;
        return card.suit == Suit::HEART && card.rank == levelRank();
    }

    bool isLevelCard(const Card& card) const {
        return !isJoker(card.rank) && card.rank == levelRank();
    }
};

Card makeCard(CardId id, Suit suit, Rank rank);

std::vector<Card> createFullDeck(int deckCount = kDeckCount);

}  // namespace guandan
