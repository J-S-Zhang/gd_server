import '../models/player.dart';
import '../models/game_mode.dart';

enum GamePhase {
  waiting,
  ready,
  dealing,
  playing,
  roundEnd,
  settlement,
  finished,
}

class Room {
  final String roomId;
  final List<Player> players;
  final bool isOwner;
  final GamePhase phase;
  final GameMode mode;
  final int maxPlayers;
  final bool enableTribute;

  const Room({
    required this.roomId,
    this.players = const [],
    this.isOwner = false,
    this.phase = GamePhase.waiting,
    this.mode = GameMode.six,
    this.maxPlayers = 6,
    this.enableTribute = false,
  });

  bool get isFull => players.length >= maxPlayers;

  bool get allReady =>
      players.length == maxPlayers && players.every((p) => p.isReady);

  bool get isSoloMode => mode == GameMode.solo;

  Room copyWith({
    String? roomId,
    List<Player>? players,
    bool? isOwner,
    GamePhase? phase,
    GameMode? mode,
    int? maxPlayers,
    bool? enableTribute,
  }) {
    return Room(
      roomId: roomId ?? this.roomId,
      players: players ?? this.players,
      isOwner: isOwner ?? this.isOwner,
      phase: phase ?? this.phase,
      mode: mode ?? this.mode,
      maxPlayers: maxPlayers ?? this.maxPlayers,
      enableTribute: enableTribute ?? this.enableTribute,
    );
  }

  static Room fromRoomData({
    required String roomId,
    required Map<String, dynamic> data,
    bool isOwner = false,
    GamePhase phase = GamePhase.waiting,
  }) {
    final mode = GameMode.fromString(data['mode'] as String?);
    final maxPlayers = data['max_players'] as int? ?? mode.maxPlayers;
    final players = (data['players'] as List<dynamic>?)
            ?.map((p) => Player.fromJson(p as Map<String, dynamic>))
            .toList() ??
        [];
    return Room(
      roomId: roomId,
      players: players,
      isOwner: isOwner,
      phase: phase,
      mode: mode,
      maxPlayers: maxPlayers,
      enableTribute: data['enable_tribute'] == true,
    );
  }
}
