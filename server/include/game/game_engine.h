#pragma once

#include "game/card.h"
#include "game/card_analyzer.h"
#include "game/game_state.h"
#include "game/rule_engine.h"
#include "game/settlement.h"
#include "game/team_progress.h"
#include "game/turn_manager.h"
#include "game/types.h"
#include <array>
#include <string>
#include <vector>

namespace guandan {

struct PlayResult {
    ErrorCode code = ErrorCode::OK;
    std::string message;
    bool gameOver = false;
    SettlementResult settlement;
};

struct PlayerView {
    GamePhase phase;
    uint64_t stateVersion;
    uint64_t turnId;
    int currentLevel;
    int attackingTeam;
    bool isPassARound;
    bool isPlayingOwnRound;
    std::array<int, 2> teamLevels{2, 2};
    std::array<bool, 2> inPassAPhase{false, false};
    std::array<int, 2> passAFailCounts{0, 0};
    int currentPlayerIndex;
    int mySeatIndex;
    std::vector<CardId> myCards;
    std::vector<CardId> lastPlayedCards;
    int lastPlayedPlayerIndex;
    CardPattern lastPattern;

    struct OtherPlayer {
        PlayerId id;
        int seatIndex;
        int team;
        int cardCount;
        bool hasFinished;
        int finishRank;
        bool isReady;
        PlayerStatus status;
    };
    std::vector<OtherPlayer> others;
};

class GameEngine {
public:
    explicit GameEngine(const GameRuleConfig& config = {});

    void init(RoomId roomId, const std::vector<PlayerId>& playerIds);
    void setPlayerReady(PlayerId playerId);
    bool allReady() const;
    PlayResult startGame();

    PlayResult playCards(PlayerId playerId, const std::vector<CardId>& cards);
    PlayResult pass(PlayerId playerId);

    bool isGameOver() const { return state_.phase == GamePhase::FINISHED; }
    const GameState& getState() const { return state_; }
    const TeamProgress& teamProgress() const { return progress_; }
    PlayerView buildViewFor(PlayerId viewerId) const;

private:
    GameRuleConfig config_;
    GameState state_;
    CardAnalyzer analyzer_;
    RuleEngine ruleEngine_;
    TurnManager turnManager_;
    Settlement settlement_;
    TeamProgress progress_;

    RuleContext ruleContext() const;
    void assignFinishRank(int playerIndex);
    bool checkTeamWin() const;
    void incrementStateVersion();
    std::vector<Card> cardsFromIds(const std::vector<CardId>& ids) const;
    int seatOf(PlayerId id) const;
};

}  // namespace guandan
