#pragma once

#include "game/card.h"
#include "game/bot_player.h"
#include "game/card_analyzer.h"
#include "game/game_state.h"
#include "game/rule_engine.h"
#include "game/settlement.h"
#include "game/team_progress.h"
#include "game/tribute.h"
#include "game/turn_manager.h"
#include "game/types.h"
#include <array>
#include <map>
#include <optional>
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
    int firstPlayerIndex;
    int mySeatIndex;
    int viewerSeatIndex = -1;
    bool isSpectating = false;
    std::vector<int> spectatableTeammates;
    std::vector<CardId> myCards;
    std::vector<CardId> lastPlayedCards;
    int lastPlayedPlayerIndex;
    CardPattern lastPattern;

    std::vector<int> pendingTributerSeats;
    std::vector<int> pendingReturnSeats;
    CardId requiredTributeCardId = 0;
    std::vector<CardId> validReturnCardIds;
    bool mustReturnTribute = false;

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
    void updateConfig(const GameRuleConfig& config);
    PlayResult startGame();
    PlayResult startNextRound();

    PlayResult playCards(PlayerId playerId, const std::vector<CardId>& cards);
    PlayResult pass(PlayerId playerId);
    PlayResult submitTribute(PlayerId playerId, CardId cardId);
    PlayResult submitReturn(PlayerId playerId, CardId cardId);

    std::optional<std::vector<CardId>> chooseBotPlay(PlayerId playerId) const;
    std::optional<CardId> chooseBotTributeCard(PlayerId playerId) const;
    std::optional<CardId> chooseBotReturnCard(PlayerId playerId) const;
    bool hasPendingTributeAction() const;
    const std::vector<int>& pendingTributerSeats() const {
        return tributeState_.pendingTributers;
    }
    const std::vector<int>& pendingReturnSeats() const {
        return tributeState_.pendingReturnSeats;
    }

    bool isGameOver() const { return state_.phase == GamePhase::FINISHED; }
    const GameState& getState() const { return state_; }
    const TeamProgress& teamProgress() const { return progress_; }
    const TributeRoundResult& lastTributeResult() const { return lastTribute_; }
    PlayerView buildViewFor(PlayerId viewerId, int anchorSeatOverride = -1) const;

private:
    GameRuleConfig config_;
    GameState state_;
    CardAnalyzer analyzer_;
    RuleEngine ruleEngine_;
    TurnManager turnManager_;
    Settlement settlement_;
    TeamProgress progress_;
    TributeManager tributeManager_;
    BotPlayer botPlayer_;
    PreviousRoundInfo previousRound_{};
    TributeRoundResult lastTribute_{};

    struct InteractiveTributeState {
        bool active = false;
        TributePhasePlan plan;
        std::vector<int> pendingTributers;
        std::map<int, CardId> tributeSubmissions;
        std::vector<int> pendingReturnSeats;
        std::map<int, int> returnToTributer;
    };
    InteractiveTributeState tributeState_{};

    RuleContext ruleContext() const;
    PlayResult dealAndStartPlaying(bool applyTribute);
    void beginPlayingFromDeal(int firstPlayerSeat);
    PlayResult finishTributePhaseAndStartPlay();
    void clearTributeState();
    void assignFinishRank(int playerIndex);
    bool checkTeamWin() const;
    void incrementStateVersion();
    std::vector<Card> cardsFromIds(const std::vector<CardId>& ids) const;
    int seatOf(PlayerId id) const;
};

}  // namespace guandan
