import 'card.dart';
import 'player.dart';
import 'room.dart';
import 'seat_round_play.dart';
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
  final int attackingTeam;
  final bool isPassARound;
  final bool isPlayingOwnRound;
  final List<int> teamLevels;
  final List<bool> inPassAPhase;
  final List<int> passAFailCounts;
  final int currentPlayerIndex;
  final int mySeatIndex;
  final int ownSeatIndex;
  final bool isSpectating;
  final List<int> spectatableTeammates;
  final List<GameCard> myCards;
  final List<int> lastPlayedCards;
  final int lastPlayedPlayerId;
  final int lastPlayedSeatIndex;
  final List<Player> players;
  final Map<int, SeatRoundPlay> seatRoundPlays;
  final List<Set<int>> handOrganizedGroups;

  const ClientGameState({
    this.phase = GamePhase.waiting,
    this.stateVersion = 0,
    this.turnId = 0,
    this.currentLevel = 2,
    this.attackingTeam = 0,
    this.isPassARound = false,
    this.isPlayingOwnRound = false,
    this.teamLevels = const [2, 2],
    this.inPassAPhase = const [false, false],
    this.passAFailCounts = const [0, 0],
    this.currentPlayerIndex = 0,
    this.mySeatIndex = 0,
    this.ownSeatIndex = 0,
    this.isSpectating = false,
    this.spectatableTeammates = const [],
    this.myCards = const [],
    this.lastPlayedCards = const [],
    this.lastPlayedPlayerId = -1,
    this.lastPlayedSeatIndex = -1,
    this.players = const [],
    this.seatRoundPlays = const {},
    this.handOrganizedGroups = const [],
  });

  bool get isMyTurn =>
      !isSpectating && currentPlayerIndex == ownSeatIndex;

  /// 本墩其他玩家均已不要，轮到自己重新领出（可出任意牌型）。
  bool canLeadFreely(int? myUserId) {
    if (lastPlayedCards.isEmpty) return true;
    if (myUserId == null || !isMyTurn) return false;
    return lastPlayedPlayerId == myUserId;
  }

  bool mustRespondToTrick(int? myUserId) =>
      lastPlayedCards.isNotEmpty && !canLeadFreely(myUserId);

  int? myTeamLevel(int myTeam) =>
      myTeam >= 0 && myTeam < teamLevels.length ? teamLevels[myTeam] : null;

  int? opponentTeamLevel(int myTeam) {
    final opponent = myTeam ^ 1;
    return opponent >= 0 && opponent < teamLevels.length
        ? teamLevels[opponent]
        : null;
  }

  int? myPassAFailCount(int myTeam) =>
      myTeam >= 0 && myTeam < passAFailCounts.length
          ? passAFailCounts[myTeam]
          : null;

  ClientGameState copyWith({
    GamePhase? phase,
    int? stateVersion,
    int? turnId,
    int? currentLevel,
    int? attackingTeam,
    bool? isPassARound,
    bool? isPlayingOwnRound,
    List<int>? teamLevels,
    List<bool>? inPassAPhase,
    List<int>? passAFailCounts,
    int? currentPlayerIndex,
    int? mySeatIndex,
    int? ownSeatIndex,
    bool? isSpectating,
    List<int>? spectatableTeammates,
    List<GameCard>? myCards,
    List<int>? lastPlayedCards,
    int? lastPlayedPlayerId,
    int? lastPlayedSeatIndex,
    List<Player>? players,
    Map<int, SeatRoundPlay>? seatRoundPlays,
    List<Set<int>>? handOrganizedGroups,
  }) {
    return ClientGameState(
      phase: phase ?? this.phase,
      stateVersion: stateVersion ?? this.stateVersion,
      turnId: turnId ?? this.turnId,
      currentLevel: currentLevel ?? this.currentLevel,
      attackingTeam: attackingTeam ?? this.attackingTeam,
      isPassARound: isPassARound ?? this.isPassARound,
      isPlayingOwnRound: isPlayingOwnRound ?? this.isPlayingOwnRound,
      teamLevels: teamLevels ?? this.teamLevels,
      inPassAPhase: inPassAPhase ?? this.inPassAPhase,
      passAFailCounts: passAFailCounts ?? this.passAFailCounts,
      currentPlayerIndex: currentPlayerIndex ?? this.currentPlayerIndex,
      mySeatIndex: mySeatIndex ?? this.mySeatIndex,
      ownSeatIndex: ownSeatIndex ?? this.ownSeatIndex,
      isSpectating: isSpectating ?? this.isSpectating,
      spectatableTeammates: spectatableTeammates ?? this.spectatableTeammates,
      myCards: myCards ?? this.myCards,
      lastPlayedCards: lastPlayedCards ?? this.lastPlayedCards,
      lastPlayedPlayerId: lastPlayedPlayerId ?? this.lastPlayedPlayerId,
      lastPlayedSeatIndex: lastPlayedSeatIndex ?? this.lastPlayedSeatIndex,
      players: players ?? this.players,
      seatRoundPlays: seatRoundPlays ?? this.seatRoundPlays,
      handOrganizedGroups: handOrganizedGroups ?? this.handOrganizedGroups,
    );
  }

  static List<int> _parseIntList(dynamic raw, List<int> fallback) {
    if (raw is! List) return fallback;
    return raw.map((e) => e as int).toList();
  }

  static List<bool> _parseBoolList(dynamic raw, List<bool> fallback) {
    if (raw is! List) return fallback;
    return raw.map((e) => e == true).toList();
  }

  static int _resolveLastPlayedPlayerId(
    int seatIndex,
    int mySeatIndex,
    List<Player> players,
  ) {
    if (seatIndex < 0) return -1;
    for (final p in players) {
      if (p.seatIndex == seatIndex) return p.id;
    }
    return -1;
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

    final mySeatIndex = json['my_seat_index'] as int? ?? 0;
    final ownSeatIndex = json['viewer_seat_index'] as int? ?? mySeatIndex;
    final spectatableTeammates = (json['spectatable_teammates'] as List<dynamic>?)
            ?.map((e) => e as int)
            .toList() ??
        const <int>[];
    final lastPlayedSeatIndex = json['last_played_player_index'] as int? ?? -1;

    return ClientGameState(
      phase: parsePhase(json['phase'] as String?),
      stateVersion: json['state_version'] as int? ?? 0,
      turnId: json['turn_id'] as int? ?? 0,
      currentLevel: json['current_level'] as int? ?? 2,
      attackingTeam: json['attacking_team'] as int? ?? 0,
      isPassARound: json['is_pass_a_round'] == true,
      isPlayingOwnRound: json['is_playing_own_round'] == true,
      teamLevels: _parseIntList(json['team_levels'], const [2, 2]),
      inPassAPhase: _parseBoolList(json['in_pass_a_phase'], const [false, false]),
      passAFailCounts: _parseIntList(json['pass_a_fail_counts'], const [0, 0]),
      currentPlayerIndex: json['current_player_index'] as int? ?? 0,
      mySeatIndex: mySeatIndex,
      ownSeatIndex: ownSeatIndex,
      isSpectating: json['is_spectating'] == true,
      spectatableTeammates: spectatableTeammates,
      myCards: myCards,
      lastPlayedCards: (json['last_played_cards'] as List<dynamic>?)
              ?.map((c) => c as int)
              .toList() ??
          [],
      lastPlayedPlayerId: _resolveLastPlayedPlayerId(
        lastPlayedSeatIndex,
        mySeatIndex,
        players,
      ),
      lastPlayedSeatIndex: lastPlayedSeatIndex,
      players: players,
    );
  }
}
