import '../models/card.dart';
import '../utils/card_pattern.dart';
import '../utils/card_utils.dart';
import '../utils/hand_layout.dart';
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../models/game_state.dart';
import '../models/player.dart';
import '../models/room.dart';
import '../models/seat_round_play.dart';
import '../network/websocket_client.dart';
import '../services/game_sound_service.dart';
import 'auth_controller.dart';
import 'room_controller.dart';

final gameStateProvider = StateProvider<ClientGameState>((ref) {
  return const ClientGameState();
});

/// 当前自己手牌区域占用高度（随叠牌层数变化而更新）。
final handCardsMaxHeightProvider = StateProvider<double>((ref) => 0);

final gameControllerProvider = Provider((ref) {
  return GameController(ref);
});

class GameController {
  final Ref _ref;
  GameController(this._ref);

  int? _lastPassSoundKey;
  List<int>? _straightFlushHandSignature;
  final Map<Suit, int> _straightFlushClickIndex = {};

  WebSocketClient get _ws => _ref.read(wsClientProvider);

  void listen() {
    final existing = _ws.onMessage;
    _ws.onMessage = (msg) {
      existing?.call(msg);
      _handleGameMessage(msg);
    };
  }

  bool playCards(String roomId, List<int> cards, {BuildContext? context}) {
    if (cards.isEmpty) return false;
    final state = _ref.read(gameStateProvider);
    final selected = state.myCards.where((c) => cards.contains(c.id)).toList();
    final myUserId = _ref.read(userProvider)?.id;
    final lastPlayed = state.canLeadFreely(myUserId)
        ? null
        : state.lastPlayedCards.map(cardFromId).toList();

    final error = validatePlaySelection(
      selected,
      state.currentLevel,
      lastPlayedCards: lastPlayed,
    );
    if (error != null) {
      if (context != null && context.mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text(error)),
        );
      }
      return false;
    }

    final removeSet = cards.toSet();

    final seatRoundPlays = _applyPlayToSeats(
      state.seatRoundPlays,
      state.mySeatIndex,
      cards,
      newTrick: state.canLeadFreely(myUserId),
    );

    final nextCards = state.myCards.where((c) => !removeSet.contains(c.id)).toList();
    _ref.read(gameStateProvider.notifier).state = state.copyWith(
      phase: GamePhase.playing,
      myCards: nextCards,
      lastPlayedCards: cards,
      lastPlayedPlayerId: myUserId ?? state.lastPlayedPlayerId,
      lastPlayedSeatIndex: state.mySeatIndex,
      seatRoundPlays: seatRoundPlays,
      handOrganizedGroups: filterOrganizedGroups(
        state.handOrganizedGroups,
        nextCards.map((c) => c.id).toSet(),
      ),
    );
    _resetStraightFlushCycleIfHandChanged(nextCards);

    _ws.playCards(roomId, cards, state.turnId);
    return true;
  }

  void pass(String roomId) {
    final state = _ref.read(gameStateProvider);
    GameSoundService.instance.playPassVoice();
    _ws.pass(roomId, state.turnId);
  }

  void toggleCardSelection(int cardId) {
    _resetStraightFlushCycleIfHandChanged(_ref.read(gameStateProvider).myCards);
    final state = _ref.read(gameStateProvider);
    final cards = state.myCards.map((c) {
      if (c.id == cardId) {
        return GameCard(
          id: c.id, suit: c.suit, rank: c.rank,
          selected: !c.selected,
        );
      }
      return c;
    }).toList();
    _ref.read(gameStateProvider.notifier).state = state.copyWith(myCards: cards);
  }

  void _resetStraightFlushCycleIfHandChanged(List<GameCard> cards) {
    final signature = cards.map((c) => c.id).toList()..sort();
    if (_straightFlushHandSignature == null ||
        !_listEquals(_straightFlushHandSignature!, signature)) {
      _straightFlushHandSignature = signature;
      _straightFlushClickIndex.clear();
    }
  }

  bool _listEquals(List<int> a, List<int> b) {
    if (a.length != b.length) return false;
    for (var i = 0; i < a.length; i++) {
      if (a[i] != b[i]) return false;
    }
    return true;
  }

  void _setSelectedCardIds(Set<int> selectedIds) {
    final state = _ref.read(gameStateProvider);
    final cards = state.myCards
        .map(
          (c) => GameCard(
            id: c.id,
            suit: c.suit,
            rank: c.rank,
            selected: selectedIds.contains(c.id),
          ),
        )
        .toList();
    _ref.read(gameStateProvider.notifier).state = state.copyWith(myCards: cards);
  }

  /// 点击高亮花色：循环选中该花色每组同花顺，全部展示后再点一次取消选中。
  void cycleStraightFlushSelection(Suit suit) {
    final state = _ref.read(gameStateProvider);
    _resetStraightFlushCycleIfHandChanged(state.myCards);

    final groups = findStraightFlushGroups(
      state.myCards,
      suit: suit,
      currentLevel: state.currentLevel,
    );
    if (groups.isEmpty) return;

    final clickIndex = _straightFlushClickIndex[suit] ?? 0;
    if (clickIndex >= groups.length) {
      _straightFlushClickIndex[suit] = 0;
      _setSelectedCardIds({});
      return;
    }

    _setSelectedCardIds(groups[clickIndex].toSet());
    _straightFlushClickIndex[suit] = clickIndex + 1;
  }

  List<int> get selectedCardIds {
    return _ref.read(gameStateProvider).myCards
        .where((c) => c.selected)
        .map((c) => c.id)
        .toList();
  }

  void sortHand(BuildContext context) {
    final state = _ref.read(gameStateProvider);
    final result = organizeHand(
      state.myCards,
      currentLevel: state.currentLevel,
      existingGroups: state.handOrganizedGroups,
    );
    if (result.message != null) {
      if (context.mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text(result.message!)),
        );
      }
      return;
    }

    _ref.read(gameStateProvider.notifier).state = state.copyWith(
      myCards: result.cards,
      handOrganizedGroups: result.organizedGroups,
    );
    _resetStraightFlushCycleIfHandChanged(result.cards);
  }

  void autoSortHand() {
    final state = _ref.read(gameStateProvider);
    final sorted = sortHandCards(state.myCards, currentLevel: state.currentLevel);
    _ref.read(gameStateProvider.notifier).state = state.copyWith(
      myCards: sorted,
      handOrganizedGroups: const [],
    );
  }

  /// 恢复理牌前的默认手牌排列（取消叠牌分组与选中状态）。
  void restoreHand() {
    final state = _ref.read(gameStateProvider);
    final sorted = sortHandCards(state.myCards, currentLevel: state.currentLevel);
    final restored = sorted
        .map(
          (c) => GameCard(
            id: c.id,
            suit: c.suit,
            rank: c.rank,
            selected: false,
          ),
        )
        .toList();
    _ref.read(gameStateProvider.notifier).state = state.copyWith(
      myCards: restored,
      handOrganizedGroups: const [],
    );
    _resetStraightFlushCycleIfHandChanged(restored);
  }

  void hint(BuildContext context) {
    final state = _ref.read(gameStateProvider);
    if (!state.isMyTurn || state.myCards.isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('当前无法提示')),
      );
      return;
    }

    final sorted = sortHandCards(state.myCards, currentLevel: state.currentLevel);
    final target = sorted.first;
    final cards = state.myCards
        .map(
          (c) => GameCard(
            id: c.id,
            suit: c.suit,
            rank: c.rank,
            selected: c.id == target.id,
          ),
        )
        .toList();
    _ref.read(gameStateProvider.notifier).state = state.copyWith(myCards: cards);
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text('提示：可出 ${target.displayName}')),
    );
  }

  void _handleGameMessage(Map<String, dynamic> msg) {
    final type = msg['type'] as String?;
    switch (type) {
      case 'game_snapshot':
      case 'cards_dealt':
        var newState = ClientGameState.fromSnapshot(
          msg['data'] as Map<String, dynamic>,
        );
        newState = _mergeRoomPlayerInfo(newState);
        newState = newState.copyWith(
          phase: GamePhase.playing,
          seatRoundPlays: const {},
          lastPlayedCards: const [],
          lastPlayedPlayerId: -1,
          lastPlayedSeatIndex: -1,
          handOrganizedGroups: const [],
        );
        _ref.read(gameStateProvider.notifier).state = newState;
        _resetStraightFlushCycleIfHandChanged(newState.myCards);
        break;
      case 'player_played':
        _applyPlayerPlayed(msg['data'] as Map<String, dynamic>? ?? {});
        break;
      case 'player_passed':
        _applyPlayerPassed(msg['data'] as Map<String, dynamic>? ?? {});
        break;
      case 'settlement':
        final data = msg['data'] as Map<String, dynamic>? ?? {};
        final matchWon = data['match_won'] == true;
        final state = _ref.read(gameStateProvider);
        _ref.read(gameStateProvider.notifier).state = state.copyWith(
          phase: matchWon ? GamePhase.settlement : GamePhase.roundEnd,
          teamLevels: _parseIntList(data['team_levels'], state.teamLevels),
          passAFailCounts:
              _parseIntList(data['pass_a_fail_counts'], state.passAFailCounts),
          players: _applyFinishRanksFromSettlement(data, state.players),
        );
        break;
      case 'tribute_resolved':
        break;
      case 'game_over':
        final state = _ref.read(gameStateProvider);
        _ref.read(gameStateProvider.notifier).state =
            state.copyWith(phase: GamePhase.finished);
        break;
      case 'room_dismissed':
        _ref.read(gameStateProvider.notifier).state = const ClientGameState();
        break;
    }
  }

  void _applyPlayerPlayed(Map<String, dynamic> data) {
    final state = _ref.read(gameStateProvider);
    final playerId = data['player_id'] as int? ?? -1;
    final playedIds = (data['cards'] as List<dynamic>?)
            ?.map((c) => c as int)
            .toList() ??
        [];
    final myUserId = _ref.read(userProvider)?.id;

    List<GameCard> myCards = state.myCards;
    if (myUserId != null && playerId == myUserId && playedIds.isNotEmpty) {
      final removeSet = playedIds.toSet();
      myCards = state.myCards.where((c) => !removeSet.contains(c.id)).toList();
    }

    final serverCardCount = data['card_count'] as int?;
    final serverHasFinished = data['has_finished'] as bool?;
    final serverFinishRank = data['finish_rank'] as int?;

    final players = state.players.map((p) {
      if (p.id != playerId || playedIds.isEmpty) return p;
      return Player(
        id: p.id,
        nickname: p.nickname,
        seatIndex: p.seatIndex,
        team: p.team,
        cardCount: serverCardCount ??
            (p.cardCount - playedIds.length).clamp(0, 999),
        hasFinished: serverHasFinished ?? p.hasFinished,
        finishRank: serverFinishRank ?? p.finishRank,
        isReady: p.isReady,
        isBot: p.isBot,
        status: p.status,
      );
    }).toList();

    int lastPlayedSeatIndex = state.lastPlayedSeatIndex;
    for (final p in players) {
      if (p.id == playerId) {
        lastPlayedSeatIndex = p.seatIndex;
        break;
      }
    }
    if (playerId >= 0) {
      final room = _ref.read(roomProvider);
      for (final p in room?.players ?? const <Player>[]) {
        if (p.id == playerId) {
          lastPlayedSeatIndex = p.seatIndex;
          break;
        }
      }
    }

    final seatRoundPlays = _applyPlayToSeats(
      state.seatRoundPlays,
      lastPlayedSeatIndex,
      playedIds,
      newTrick: state.lastPlayedCards.isEmpty && playedIds.isNotEmpty,
    );

    _ref.read(gameStateProvider.notifier).state = state.copyWith(
      phase: GamePhase.playing,
      lastPlayedCards: playedIds,
      lastPlayedPlayerId: playerId,
      lastPlayedSeatIndex: lastPlayedSeatIndex,
      myCards: myCards,
      players: players,
      seatRoundPlays: seatRoundPlays,
      handOrganizedGroups: filterOrganizedGroups(
        state.handOrganizedGroups,
        myCards.map((c) => c.id).toSet(),
      ),
      stateVersion: data['state_version'] as int? ?? state.stateVersion,
      turnId: data['turn_id'] as int? ?? state.turnId,
      currentPlayerIndex: data['next_player'] as int? ?? state.currentPlayerIndex,
    );
  }

  void _applyPlayerPassed(Map<String, dynamic> data) {
    final state = _ref.read(gameStateProvider);
    final roundReset = data['round_reset'] == true;
    final passSeat = _resolvePassSeat(data, state);
    _playPassVoiceIfNeeded(data);

    final nextPlayer = data['next_player'] as int? ?? state.currentPlayerIndex;
    final leadAgain = !roundReset &&
        state.lastPlayedSeatIndex >= 0 &&
        nextPlayer == state.lastPlayedSeatIndex;
    final clearTrick = roundReset || leadAgain;

    _ref.read(gameStateProvider.notifier).state = state.copyWith(
      seatRoundPlays: _applyPassToSeats(
        state.seatRoundPlays,
        passSeat,
        roundReset: clearTrick,
      ),
      lastPlayedCards: clearTrick ? const [] : null,
      lastPlayedPlayerId: clearTrick ? -1 : null,
      lastPlayedSeatIndex: clearTrick ? -1 : null,
      stateVersion: data['state_version'] as int? ?? state.stateVersion,
      turnId: data['turn_id'] as int? ?? state.turnId,
      currentPlayerIndex: nextPlayer,
    );
  }

  void _playPassVoiceIfNeeded(Map<String, dynamic> data) {
    final playerId = data['player_id'] as int? ?? -1;
    final myUserId = _ref.read(userProvider)?.id;
    if (myUserId != null && playerId == myUserId) return;

    final stateVersion = data['state_version'] as int? ?? 0;
    final soundKey = Object.hash(playerId, stateVersion);
    if (_lastPassSoundKey == soundKey) return;
    _lastPassSoundKey = soundKey;
    GameSoundService.instance.playPassVoice();
  }

  int _resolvePassSeat(Map<String, dynamic> data, ClientGameState state) {
    final seatIndex = data['seat_index'] as int?;
    if (seatIndex != null && seatIndex >= 0) return seatIndex;

    final playerId = data['player_id'] as int? ?? -1;
    if (playerId >= 0) {
      for (final p in state.players) {
        if (p.id == playerId) return p.seatIndex;
      }
      final room = _ref.read(roomProvider);
      for (final p in room?.players ?? const <Player>[]) {
        if (p.id == playerId) return p.seatIndex;
      }
    }
    return -1;
  }

  Map<int, SeatRoundPlay> _applyPlayToSeats(
    Map<int, SeatRoundPlay> current,
    int seatIndex,
    List<int> cardIds, {
    required bool newTrick,
  }) {
    if (seatIndex < 0 || cardIds.isEmpty) return current;
    final next = newTrick
        ? <int, SeatRoundPlay>{}
        : Map<int, SeatRoundPlay>.from(current);
    next[seatIndex] = SeatRoundPlay(cardIds: cardIds);
    return next;
  }

  Map<int, SeatRoundPlay> _applyPassToSeats(
    Map<int, SeatRoundPlay> current,
    int seatIndex, {
    required bool roundReset,
  }) {
    if (roundReset) return {};
    if (seatIndex < 0) return current;
    final next = Map<int, SeatRoundPlay>.from(current);
    next[seatIndex] = const SeatRoundPlay.passed();
    return next;
  }

  Map<int, SeatRoundPlay> _seatPlaysFromSnapshot(ClientGameState state) {
    if (state.lastPlayedCards.isEmpty || state.lastPlayedSeatIndex < 0) {
      return {};
    }
    return {
      state.lastPlayedSeatIndex: SeatRoundPlay(cardIds: state.lastPlayedCards),
    };
  }

  List<Player> _applyFinishRanksFromSettlement(
    Map<String, dynamic> data,
    List<Player> players,
  ) {
    final ranks = data['finish_ranks'];
    if (ranks is! List || ranks.isEmpty) return players;

    final rankById = <int, Map<String, dynamic>>{};
    for (final item in ranks) {
      if (item is! Map<String, dynamic>) continue;
      final id = item['player_id'] as int?;
      if (id != null) rankById[id] = item;
    }
    if (rankById.isEmpty) return players;

    return players.map((p) {
      final info = rankById[p.id];
      if (info == null) return p;
      return Player(
        id: p.id,
        nickname: p.nickname,
        seatIndex: p.seatIndex,
        team: p.team,
        cardCount: 0,
        hasFinished: info['has_finished'] as bool? ?? true,
        finishRank: info['finish_rank'] as int? ?? p.finishRank,
        isReady: p.isReady,
        isBot: p.isBot,
        status: p.status,
      );
    }).toList();
  }

  List<int> _parseIntList(dynamic raw, List<int> fallback) {
    if (raw is! List) return fallback;
    return raw.map((e) => e as int).toList();
  }

  ClientGameState _mergeRoomPlayerInfo(ClientGameState state) {
    final room = _ref.read(roomProvider);
    if (room == null) return state;

    final nicknames = {for (final p in room.players) p.id: p.nickname};
    final players = state.players.map((p) {
      final nick = nicknames[p.id];
      if (nick == null) return p;
      return Player(
        id: p.id,
        nickname: nick,
        seatIndex: p.seatIndex,
        team: p.team,
        cardCount: p.cardCount,
        hasFinished: p.hasFinished,
        finishRank: p.finishRank,
        isReady: p.isReady,
        isBot: p.isBot,
        status: p.status,
      );
    }).toList();
    return state.copyWith(players: players);
  }
}
