#include "game/game_engine.h"
#include "game/deck.h"

namespace guandan {

GameEngine::GameEngine(const GameRuleConfig& config)
    : config_(config), turnManager_(state_) {}

void GameEngine::init(RoomId roomId, const std::vector<PlayerId>& playerIds) {
    state_ = GameState{};
    state_.roomId = roomId;
    state_.playerCount = static_cast<int>(playerIds.size());
    state_.phase = GamePhase::WAITING;
    state_.currentLevel = 2;

    Deck deck(config_.deckCount);
    state_.allCards = deck.cards();

    for (int i = 0; i < state_.playerCount; ++i) {
        state_.players[i].id = playerIds[i];
        state_.players[i].seatIndex = i;
        state_.players[i].team = i % 2;
        state_.players[i].isReady = false;
        state_.players[i].hasFinished = false;
    }
}

void GameEngine::setPlayerReady(PlayerId playerId) {
    auto* p = state_.getPlayerById(playerId);
    if (p) p->isReady = true;
}

bool GameEngine::allReady() const {
    for (int i = 0; i < state_.playerCount; ++i) {
        if (!state_.players[i].isReady) return false;
    }
    return state_.playerCount == config_.playerCount;
}

RuleContext GameEngine::ruleContext() const {
    RuleContext ctx;
    ctx.currentLevel = state_.currentLevel;
    ctx.enableWildCard = config_.enableWildCard;
    return ctx;
}

void GameEngine::incrementStateVersion() {
    state_.stateVersion++;
}

std::vector<Card> GameEngine::cardsFromIds(const std::vector<CardId>& ids) const {
    std::vector<Card> result;
    result.reserve(ids.size());
    for (CardId id : ids) {
        result.push_back(state_.getCardById(id));
    }
    return result;
}

int GameEngine::seatOf(PlayerId id) const {
    for (int i = 0; i < state_.playerCount; ++i) {
        if (state_.players[i].id == id) return i;
    }
    return -1;
}

PlayResult GameEngine::startGame() {
    PlayResult result;
    if (state_.phase != GamePhase::WAITING && state_.phase != GamePhase::READY) {
        result.code = ErrorCode::INVALID_STATE;
        result.message = "Cannot start game in current phase";
        return result;
    }
    if (!allReady()) {
        result.code = ErrorCode::NOT_ALL_READY;
        result.message = "Not all players ready";
        return result;
    }

    Deck deck(config_.deckCount);
    deck.shuffle();
    auto hands = deck.deal(state_.playerCount);

    for (int i = 0; i < state_.playerCount; ++i) {
        state_.players[i].hand = Hand{};
        state_.players[i].hand.addAll(hands[i]);
        state_.players[i].hasFinished = false;
        state_.players[i].finishRank = 0;
    }

    state_.phase = GamePhase::PLAYING;
    state_.round++;
    state_.firstPlayerIndex = 0;
    state_.currentPlayerIndex = 0;
    state_.lastPlayedCards.clear();
    state_.lastPattern = CardPattern::invalid();
    state_.lastPlayedPlayerIndex = -1;
    state_.passCount = 0;
    state_.turnId = 1;
    incrementStateVersion();

    return result;
}

void GameEngine::assignFinishRank(int playerIndex) {
    int rank = 1;
    for (const auto& p : state_.players) {
        if (p.hasFinished) ++rank;
    }
    state_.players[playerIndex].hasFinished = true;
    state_.players[playerIndex].finishRank = rank;
}

bool GameEngine::checkTeamWin() const {
    for (int team = 0; team < kTeamCount; ++team) {
        int finished = 0;
        for (const auto& p : state_.players) {
            if (p.team == team && p.hasFinished) ++finished;
        }
        if (finished >= kPlayersPerTeam) return true;
    }
    return false;
}

PlayResult GameEngine::playCards(PlayerId playerId, const std::vector<CardId>& cards) {
    PlayResult result;
    if (state_.phase != GamePhase::PLAYING) {
        result.code = ErrorCode::INVALID_STATE;
        return result;
    }

    int seat = seatOf(playerId);
    if (seat < 0) {
        result.code = ErrorCode::INVALID_STATE;
        return result;
    }
    if (seat != state_.currentPlayerIndex) {
        result.code = ErrorCode::NOT_YOUR_TURN;
        return result;
    }
    if (state_.players[seat].hasFinished) {
        result.code = ErrorCode::INVALID_STATE;
        return result;
    }
    if (!state_.players[seat].hand.contains(cards)) {
        result.code = ErrorCode::INVALID_CARDS;
        return result;
    }

    auto cardObjs = cardsFromIds(cards);
    auto pattern = analyzer_.analyze(cardObjs, ruleContext());
    if (!pattern.isValid) {
        result.code = ErrorCode::INVALID_PATTERN;
        return result;
    }

    if (state_.lastPattern.isValid) {
        if (!ruleEngine_.canBeat(pattern, state_.lastPattern, ruleContext())) {
            result.code = ErrorCode::CANNOT_BEAT;
            return result;
        }
    }

    state_.players[seat].hand.remove(cards);
    state_.lastPlayedCards = cards;
    state_.lastPattern = pattern;
    state_.lastPlayedPlayerIndex = seat;
    state_.passCount = 0;
    incrementStateVersion();

    if (state_.players[seat].hand.empty()) {
        assignFinishRank(seat);
        if (checkTeamWin()) {
            state_.phase = GamePhase::FINISHED;
            result.gameOver = true;
            result.settlement = settlement_.calculate(state_);
            return result;
        }
    }

    turnManager_.advanceTurn();

    // Skip finished players
    while (state_.players[state_.currentPlayerIndex].hasFinished) {
        turnManager_.advanceTurn();
    }

    return result;
}

PlayResult GameEngine::pass(PlayerId playerId) {
    PlayResult result;
    if (state_.phase != GamePhase::PLAYING) {
        result.code = ErrorCode::INVALID_STATE;
        return result;
    }

    int seat = seatOf(playerId);
    if (seat < 0 || seat != state_.currentPlayerIndex) {
        result.code = ErrorCode::NOT_YOUR_TURN;
        return result;
    }

    if (!state_.lastPattern.isValid) {
        result.code = ErrorCode::INVALID_STATE;
        result.message = "Must play on new round";
        return result;
    }

    state_.passCount++;
    incrementStateVersion();

    if (turnManager_.allOthersPassed()) {
        int winner = state_.lastPlayedPlayerIndex;
        turnManager_.resetRound(winner);
        while (state_.players[state_.currentPlayerIndex].hasFinished) {
            turnManager_.advanceTurn();
        }
    } else {
        turnManager_.advanceTurn();
        while (state_.players[state_.currentPlayerIndex].hasFinished) {
            turnManager_.advanceTurn();
        }
    }

    return result;
}

PlayerView GameEngine::buildViewFor(PlayerId viewerId) const {
    PlayerView view;
    view.phase = state_.phase;
    view.stateVersion = state_.stateVersion;
    view.turnId = state_.turnId;
    view.currentLevel = state_.currentLevel;
    view.currentPlayerIndex = state_.currentPlayerIndex;
    view.lastPlayedCards = state_.lastPlayedCards;
    view.lastPlayedPlayerIndex = state_.lastPlayedPlayerIndex;
    view.lastPattern = state_.lastPattern;

    int viewerSeat = seatOf(viewerId);
    view.mySeatIndex = viewerSeat;

    if (viewerSeat >= 0) {
        view.myCards = state_.players[viewerSeat].hand.cards();
    }

    for (int i = 0; i < state_.playerCount; ++i) {
        if (i == viewerSeat) continue;
        PlayerView::OtherPlayer op;
        op.id = state_.players[i].id;
        op.seatIndex = i;
        op.team = state_.players[i].team;
        op.cardCount = static_cast<int>(state_.players[i].hand.size());
        op.hasFinished = state_.players[i].hasFinished;
        op.finishRank = state_.players[i].finishRank;
        op.isReady = state_.players[i].isReady;
        op.status = state_.players[i].status;
        view.others.push_back(op);
    }

    return view;
}

}  // namespace guandan
