import 'card.dart';
import 'player.dart';
import 'room.dart';
import '../utils/card_utils.dart';

GamePhase parsePhase(String? phase) {
  if (phase == null) return GamePhase.waiting;
  switch (phase.toUpperCase()) {
    case 'WAITING':
      return GamePhase.waiting;
    case 'READY':
      return GamePhase.ready;
    case 'DEALING':
      return GamePhase.dealing;
    case 'PLAYING':
      return GamePhase.playing;
    case 'ROUND_END':
      return GamePhase.roundEnd;
    case 'SETTLEMENT':
      return GamePhase.settlement;
    case 'FINISHED':
      return GamePhase.finished;
    default:
      return GamePhase.waiting;
  }
}

class ClientGameState {
  final GamePhase phase;
  final int stateVersion;
  final int turnId;
  final int currentLevel;
  final int currentPlayerIndex;
  final int mySeatIndex;
  final List<GameCard> myCards;
  final List<int> lastPlayedCards;
  final int lastPlayedPlayerIndex;
  final List<Player> players;

  const ClientGameState({
    this.phase = GamePhase.waiting,
    this.stateVersion = 0,
    this.turnId = 0,
    this.currentLevel = 2,
    this.currentPlayerIndex = 0,
    this.mySeatIndex = 0,
    this.myCards = const [],
    this.lastPlayedCards = const [],
    this.lastPlayedPlayerIndex = -1,
    this.players = const [],
  });

  bool get isMyTurn => currentPlayerIndex == mySeatIndex;

  ClientGameState copyWith({
    GamePhase? phase,
    int? stateVersion,
    int? turnId,
    int? currentLevel,
    int? currentPlayerIndex,
    int? mySeatIndex,
    List<GameCard>? myCards,
    List<int>? lastPlayedCards,
    int? lastPlayedPlayerIndex,
    List<Player>? players,
  }) {
    return ClientGameState(
      phase: phase ?? this.phase,
      stateVersion: stateVersion ?? this.stateVersion,
      turnId: turnId ?? this.turnId,
      currentLevel: currentLevel ?? this.currentLevel,
      currentPlayerIndex: currentPlayerIndex ?? this.currentPlayerIndex,
      mySeatIndex: mySeatIndex ?? this.mySeatIndex,
      myCards: myCards ?? this.myCards,
      lastPlayedCards: lastPlayedCards ?? this.lastPlayedCards,
      lastPlayedPlayerIndex: lastPlayedPlayerIndex ?? this.lastPlayedPlayerIndex,
      players: players ?? this.players,
    );
  }

  factory ClientGameState.fromSnapshot(Map<String, dynamic> json) {
    List<GameCard> myCards;
    final rawCards = json['my_cards'];
    if (rawCards is List) {
      if (rawCards.isNotEmpty && rawCards.first is Map) {
        myCards = rawCards
            .map((c) => GameCard.fromJson(c as Map<String, dynamic>))
            .toList();
      } else {
        myCards = cardsFromIds(rawCards.cast<int>());
      }
    } else {
      myCards = [];
    }

    final players = (json['players'] as List<dynamic>?)
            ?.map((p) => Player.fromJson(p as Map<String, dynamic>))
            .toList() ??
        [];

    return ClientGameState(
      phase: parsePhase(json['phase'] as String?),
      stateVersion: json['state_version'] as int? ?? 0,
      turnId: json['turn_id'] as int? ?? 0,
      currentLevel: json['current_level'] as int? ?? 2,
      currentPlayerIndex: json['current_player_index'] as int? ?? 0,
      mySeatIndex: json['my_seat_index'] as int? ?? 0,
      myCards: myCards,
      lastPlayedCards: (json['last_played_cards'] as List<dynamic>?)
              ?.map((c) => c as int)
              .toList() ??
          [],
      lastPlayedPlayerIndex: json['last_played_player_index'] as int? ?? -1,
      players: players,
    );
  }
}
