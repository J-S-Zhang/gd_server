#include "game/game_engine.h"
#include "game/deck.h"
#include "game/finish_rank.h"
#include <algorithm>

namespace guandan {

GameEngine::GameEngine(const GameRuleConfig& config)
    : config_(config), turnManager_(state_) {}

void GameEngine::init(RoomId roomId, const std::vector<PlayerId>& playerIds) {
    state_ = GameState{};
    state_.roomId = roomId;
    state_.playerCount = static_cast<int>(playerIds.size());
    state_.phase = GamePhase::WAITING;

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

void GameEngine::updateConfig(const GameRuleConfig& config) {
    if (state_.phase == GamePhase::PLAYING || state_.phase == GamePhase::FINISHED ||
        state_.phase == GamePhase::TRIBUTE || state_.phase == GamePhase::RETURN_TRIBUTE) {
        return;
    }
    config_ = config;
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

PlayResult GameEngine::dealAndStartPlaying(bool applyTribute) {
    PlayResult result;

    Deck deck(config_.deckCount);
    deck.shuffle();
    state_.allCards = deck.cards();

    const int cardsPerPlayer =
        (config_.ruleVersion == "solo_test_v1") ? kSoloTestCardsPerPlayer : 0;
    auto hands = deck.deal(state_.playerCount, cardsPerPlayer);

    for (int i = 0; i < state_.playerCount; ++i) {
        state_.players[i].hand = Hand{};
        state_.players[i].hand.addAll(hands[i]);
        state_.players[i].hasFinished = false;
        state_.players[i].finishRank = 0;
    }

    progress_.prepareNextRound();
    state_.attackingTeam = progress_.attackingTeam;
    state_.currentLevel = progress_.currentRoundLevel();
    state_.isPassARound = progress_.isPassARound();

    state_.round++;
    state_.lastPlayedCards.clear();
    state_.lastPattern = CardPattern::invalid();
    state_.lastPlayedPlayerIndex = -1;
    state_.passCount = 0;
    state_.turnId = 1;

    if (applyTribute) {
        clearTributeState();
        const auto plan = tributeManager_.planRound(state_, config_, previousRound_);
        if (plan.skipped) {
            lastTribute_ = {};
            lastTribute_.skipped = true;
            lastTribute_.firstPlayerSeat = plan.headSeat;
            beginPlayingFromDeal(plan.headSeat);
            return result;
        }
        if (plan.antiTribute) {
            lastTribute_ = {};
            lastTribute_.antiTribute = true;
            lastTribute_.firstPlayerSeat = plan.headSeat;
            lastTribute_.summary = plan.summary;
            beginPlayingFromDeal(plan.headSeat);
            return result;
        }

        tributeState_.active = true;
        tributeState_.plan = plan;
        tributeState_.pendingTributers = plan.tributerSeats;
        lastTribute_ = {};
        state_.phase = GamePhase::TRIBUTE;
        incrementStateVersion();
        return result;
    }

    lastTribute_ = {};
    lastTribute_.skipped = true;
    beginPlayingFromDeal(0);
    return result;
}

void GameEngine::clearTributeState() {
    tributeState_ = {};
}

void GameEngine::beginPlayingFromDeal(int firstPlayerSeat) {
    state_.phase = GamePhase::PLAYING;
    state_.firstPlayerIndex = firstPlayerSeat;
    state_.currentPlayerIndex = firstPlayerSeat;
    incrementStateVersion();
}

PlayResult GameEngine::finishTributePhaseAndStartPlay() {
    PlayResult result;
    const int firstPlayerSeat = tributeManager_.computeFirstPlayerSeat(
        lastTribute_, tributeState_.plan.headSeat);
    lastTribute_.firstPlayerSeat = firstPlayerSeat;
    lastTribute_.headTributerSeat = -1;
    for (const auto& tr : lastTribute_.tributes) {
        if (tr.toSeat == tributeState_.plan.headSeat) {
            lastTribute_.headTributerSeat = tr.fromSeat;
            break;
        }
    }
    clearTributeState();
    beginPlayingFromDeal(firstPlayerSeat);
    return result;
}

bool GameEngine::hasPendingTributeAction() const {
    return tributeState_.active &&
           (state_.phase == GamePhase::TRIBUTE ||
            state_.phase == GamePhase::RETURN_TRIBUTE);
}

PlayResult GameEngine::submitTribute(PlayerId playerId, CardId cardId) {
    PlayResult result;
    if (state_.phase != GamePhase::TRIBUTE || !tributeState_.active) {
        result.code = ErrorCode::INVALID_STATE;
        return result;
    }

    const int seat = seatOf(playerId);
    if (seat < 0) {
        result.code = ErrorCode::INVALID_STATE;
        return result;
    }

    const auto& pending = tributeState_.pendingTributers;
    if (std::find(pending.begin(), pending.end(), seat) == pending.end()) {
        result.code = ErrorCode::NOT_YOUR_TURN;
        return result;
    }

    if (!tributeManager_.isValidTributeSubmission(seat, cardId, state_, ruleContext())) {
        result.code = ErrorCode::INVALID_CARDS;
        result.message = "Must tribute the largest non-wild card";
        return result;
    }

    tributeState_.tributeSubmissions[seat] = cardId;
    tributeState_.pendingTributers.erase(
        std::remove(tributeState_.pendingTributers.begin(),
                    tributeState_.pendingTributers.end(), seat),
        tributeState_.pendingTributers.end());
    incrementStateVersion();

    if (!tributeState_.pendingTributers.empty()) {
        return result;
    }

    lastTribute_.skipped = false;
    lastTribute_.antiTribute = false;
    tributeManager_.assignTributes(
        state_, ruleContext(), tributeState_.plan,
        tributeState_.tributeSubmissions, lastTribute_);

    if (lastTribute_.tributes.empty()) {
        lastTribute_.firstPlayerSeat = tributeState_.plan.headSeat;
        return finishTributePhaseAndStartPlay();
    }

    tributeState_.pendingReturnSeats.clear();
    tributeState_.returnToTributer.clear();
    for (const auto& tr : lastTribute_.tributes) {
        tributeState_.pendingReturnSeats.push_back(tr.toSeat);
        tributeState_.returnToTributer[tr.toSeat] = tr.fromSeat;
    }
    state_.phase = GamePhase::RETURN_TRIBUTE;
    incrementStateVersion();
    return result;
}

PlayResult GameEngine::submitReturn(PlayerId playerId, CardId cardId) {
    PlayResult result;
    if (state_.phase != GamePhase::RETURN_TRIBUTE || !tributeState_.active) {
        result.code = ErrorCode::INVALID_STATE;
        return result;
    }

    const int seat = seatOf(playerId);
    if (seat < 0) {
        result.code = ErrorCode::INVALID_STATE;
        return result;
    }

    const auto& pending = tributeState_.pendingReturnSeats;
    if (std::find(pending.begin(), pending.end(), seat) == pending.end()) {
        result.code = ErrorCode::NOT_YOUR_TURN;
        return result;
    }

    if (!tributeManager_.isValidReturnSubmission(seat, cardId, state_, ruleContext())) {
        result.code = ErrorCode::INVALID_CARDS;
        result.message = "Invalid return card";
        return result;
    }

    const int toSeat = tributeState_.returnToTributer[seat];
    tributeManager_.applyReturnTransfer(state_, seat, toSeat, cardId);
    lastTribute_.returns.push_back({seat, toSeat, cardId});

    tributeState_.pendingReturnSeats.erase(
        std::remove(tributeState_.pendingReturnSeats.begin(),
                    tributeState_.pendingReturnSeats.end(), seat),
        tributeState_.pendingReturnSeats.end());
    incrementStateVersion();

    if (!tributeState_.pendingReturnSeats.empty()) {
        return result;
    }

    return finishTributePhaseAndStartPlay();
}

std::optional<CardId> GameEngine::chooseBotTributeCard(PlayerId playerId) const {
    const int seat = seatOf(playerId);
    if (seat < 0) return std::nullopt;
    const CardId cardId = tributeManager_.requiredTributeCard(
        state_.players[seat].hand, state_, ruleContext());
    return cardId == 0 ? std::nullopt : std::optional<CardId>(cardId);
}

std::optional<CardId> GameEngine::chooseBotReturnCard(PlayerId playerId) const {
    const int seat = seatOf(playerId);
    if (seat < 0) return std::nullopt;
    const CardId cardId = tributeManager_.pickReturnCard(
        state_.players[seat].hand, state_, ruleContext());
    return cardId == 0 ? std::nullopt : std::optional<CardId>(cardId);
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

    previousRound_ = {};
    return dealAndStartPlaying(false);
}

PlayResult GameEngine::startNextRound() {
    PlayResult result;
    if (state_.phase != GamePhase::FINISHED) {
        result.code = ErrorCode::INVALID_STATE;
        result.message = "Cannot start next round in current phase";
        return result;
    }

    return dealAndStartPlaying(true);
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
        if (finished >= config_.playersPerTeam) return true;
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
            assignRemainingFinishRanks(state_);
            result.gameOver = true;
            result.settlement = settlement_.calculate(state_, config_.playersPerTeam);
            progress_.applySettlement(result.settlement);
            previousRound_ = buildPreviousRoundInfo(state_, result.settlement.winningTeam);
            state_.phase = GamePhase::FINISHED;
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

std::optional<std::vector<CardId>> GameEngine::chooseBotPlay(PlayerId playerId) const {
    const int seat = seatOf(playerId);
    if (seat < 0) return std::nullopt;
    return botPlayer_.choosePlay(state_, seat, ruleContext(), analyzer_, ruleEngine_);
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

    const int leader = state_.lastPlayedPlayerIndex;
    if (leader >= 0 &&
        (turnManager_.shouldResetRoundAfterPass(seat) ||
         turnManager_.allOthersPassed())) {
        turnManager_.resetRound(leader);
    } else {
        turnManager_.advanceTurn();
    }

    while (state_.players[state_.currentPlayerIndex].hasFinished) {
        turnManager_.advanceTurn();
    }

    return result;
}

PlayerView GameEngine::buildViewFor(PlayerId viewerId, int anchorSeatOverride) const {
    PlayerView view;
    view.phase = state_.phase;
    view.stateVersion = state_.stateVersion;
    view.turnId = state_.turnId;
    view.currentLevel = state_.currentLevel;
    view.attackingTeam = state_.attackingTeam;
    view.isPassARound = state_.isPassARound;
    view.teamLevels = progress_.levels;
    view.inPassAPhase = progress_.inPassAPhase;
    view.passAFailCounts = progress_.passAFailCounts;
    view.currentPlayerIndex = state_.currentPlayerIndex;
    view.firstPlayerIndex = state_.firstPlayerIndex;
    view.lastPlayedCards = state_.lastPlayedCards;
    view.lastPlayedPlayerIndex = state_.lastPlayedPlayerIndex;
    view.lastPattern = state_.lastPattern;

    const int viewerSeat = seatOf(viewerId);
    view.viewerSeatIndex = viewerSeat;

    int anchorSeat = viewerSeat;
    if (anchorSeatOverride >= 0 && anchorSeatOverride < state_.playerCount) {
        anchorSeat = anchorSeatOverride;
    }

    view.mySeatIndex = anchorSeat;
    view.isSpectating = viewerSeat >= 0 && anchorSeat != viewerSeat;

    if (viewerSeat >= 0 && state_.players[viewerSeat].hasFinished) {
        const int team = state_.players[viewerSeat].team;
        for (int i = 0; i < state_.playerCount; ++i) {
            if (i == viewerSeat) continue;
            if (state_.players[i].team == team && !state_.players[i].hasFinished) {
                view.spectatableTeammates.push_back(i);
            }
        }
    }

    if (anchorSeat >= 0) {
        view.myCards = state_.players[anchorSeat].hand.cards();
        const int anchorTeam = state_.players[anchorSeat].team;
        view.isPlayingOwnRound = progress_.isPlayingOwnRound(anchorTeam);
    }

    if (tributeState_.active) {
        view.pendingTributerSeats = tributeState_.pendingTributers;
        view.pendingReturnSeats = tributeState_.pendingReturnSeats;
        if (viewerSeat >= 0) {
            if (state_.phase == GamePhase::TRIBUTE) {
                const bool isPendingTributer = std::find(
                    tributeState_.pendingTributers.begin(),
                    tributeState_.pendingTributers.end(),
                    viewerSeat) != tributeState_.pendingTributers.end();
                if (isPendingTributer) {
                    view.requiredTributeCardId = tributeManager_.requiredTributeCard(
                        state_.players[viewerSeat].hand, state_, ruleContext());
                }
            } else if (state_.phase == GamePhase::RETURN_TRIBUTE) {
                const bool isPendingReturner = std::find(
                    tributeState_.pendingReturnSeats.begin(),
                    tributeState_.pendingReturnSeats.end(),
                    viewerSeat) != tributeState_.pendingReturnSeats.end();
                view.mustReturnTribute = isPendingReturner;
                if (isPendingReturner) {
                    view.validReturnCardIds = tributeManager_.validReturnCardIds(
                        state_.players[viewerSeat].hand, state_, ruleContext());
                }
            }
        }
    }

    for (int i = 0; i < state_.playerCount; ++i) {
        if (i == anchorSeat) continue;
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
