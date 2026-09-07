import '../models/card.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../models/game_state.dart';
import '../models/room.dart';
import '../network/websocket_client.dart';
import 'room_controller.dart';

final gameStateProvider = StateProvider<ClientGameState>((ref) {
  return const ClientGameState();
});

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

  void playCards(String roomId, List<int> cards) {
    final state = _ref.read(gameStateProvider);
    _ws.playCards(roomId, cards, state.turnId);
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

  void _handleGameMessage(Map<String, dynamic> msg) {
    final type = msg['type'] as String?;
    switch (type) {
      case 'game_snapshot':
      case 'cards_dealt':
        _ref.read(gameStateProvider.notifier).state =
            ClientGameState.fromSnapshot(msg['data'] as Map<String, dynamic>);
        break;
      case 'player_played':
        final data = msg['data'] as Map<String, dynamic>;
        final state = _ref.read(gameStateProvider);
        _ref.read(gameStateProvider.notifier).state = state.copyWith(
          lastPlayedCards: (data['cards'] as List).cast<int>(),
          lastPlayedPlayerIndex: data['player_id'] as int? ?? -1,
          stateVersion: data['state_version'] as int? ?? state.stateVersion,
          turnId: data['turn_id'] as int? ?? state.turnId,
          currentPlayerIndex: data['next_player'] as int? ?? state.currentPlayerIndex,
        );
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
}
