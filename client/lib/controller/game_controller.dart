import '../models/card.dart';
import '../utils/card_pattern.dart';
import '../utils/card_utils.dart';
import '../utils/hand_layout.dart';
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../models/game_state.dart';
import '../models/player.dart';
import '../models/room.dart';
import '../network/websocket_client.dart';
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
    final lastPlayed = state.lastPlayedCards.isEmpty
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

    final myUserId = _ref.read(userProvider)?.id;
    final removeSet = cards.toSet();

    _ref.read(gameStateProvider.notifier).state = state.copyWith(
      phase: GamePhase.playing,
      myCards: state.myCards.where((c) => !removeSet.contains(c.id)).toList(),
      lastPlayedCards: cards,
      lastPlayedPlayerId: myUserId ?? state.lastPlayedPlayerId,
      lastPlayedSeatIndex: state.mySeatIndex,
      handStraightStackIds: state.handStraightStackIds
          .where((id) => !removeSet.contains(id))
          .toSet(),
    );

    _ws.playCards(roomId, cards, state.turnId);
    return true;
  }

  void pass(String roomId) {
    final state = _ref.read(gameStateProvider);
    _ws.pass(roomId, state.turnId);
  }

  void toggleCardSelection(int cardId) {
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
      tryStraightFromSelection: true,
    );
    _ref.read(gameStateProvider.notifier).state = state.copyWith(
      myCards: result.cards,
      handStraightStackIds: result.straightStackIds,
    );
    if (result.message != null && context.mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text(result.message!)),
      );
    }
  }

  void autoSortHand() {
    final state = _ref.read(gameStateProvider);
    final result = organizeHand(
      state.myCards,
      currentLevel: state.currentLevel,
      tryStraightFromSelection: false,
    );
    _ref.read(gameStateProvider.notifier).state = state.copyWith(
      myCards: result.cards,
      handStraightStackIds: const {},
    );
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
        _ref.read(gameStateProvider.notifier).state = newState;
        break;
      case 'player_played':
        _applyPlayerPlayed(msg['data'] as Map<String, dynamic>? ?? {});
        break;
      case 'player_passed':
        final data = msg['data'] as Map<String, dynamic>? ?? {};
        final state = _ref.read(gameStateProvider);
        _ref.read(gameStateProvider.notifier).state = state.copyWith(
          stateVersion: data['state_version'] as int? ?? state.stateVersion,
          turnId: data['turn_id'] as int? ?? state.turnId,
          currentPlayerIndex: data['next_player'] as int? ?? state.currentPlayerIndex,
        );
        break;
      case 'settlement':
        final state = _ref.read(gameStateProvider);
        _ref.read(gameStateProvider.notifier).state =
            state.copyWith(phase: GamePhase.settlement);
        break;
      case 'game_over':
        final state = _ref.read(gameStateProvider);
        _ref.read(gameStateProvider.notifier).state =
            state.copyWith(phase: GamePhase.finished);
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

    _ref.read(gameStateProvider.notifier).state = state.copyWith(
      phase: GamePhase.playing,
      lastPlayedCards: playedIds,
      lastPlayedPlayerId: playerId,
      lastPlayedSeatIndex: lastPlayedSeatIndex,
      myCards: myCards,
      players: players,
      handStraightStackIds: state.handStraightStackIds
          .where((id) => myCards.any((c) => c.id == id))
          .toSet(),
      stateVersion: data['state_version'] as int? ?? state.stateVersion,
      turnId: data['turn_id'] as int? ?? state.turnId,
      currentPlayerIndex: data['next_player'] as int? ?? state.currentPlayerIndex,
    );
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
