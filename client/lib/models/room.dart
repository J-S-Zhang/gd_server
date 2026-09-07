import 'player.dart';

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

  const Room({
    required this.roomId,
    this.players = const [],
    this.isOwner = false,
    this.phase = GamePhase.waiting,
  });

  bool get isFull => players.length >= 6;
  bool get allReady => players.length == 6 && players.every((p) => p.isReady);

  Room copyWith({
    String? roomId,
    List<Player>? players,
    bool? isOwner,
    GamePhase? phase,
  }) {
    return Room(
      roomId: roomId ?? this.roomId,
      players: players ?? this.players,
      isOwner: isOwner ?? this.isOwner,
      phase: phase ?? this.phase,
    );
  }
}
