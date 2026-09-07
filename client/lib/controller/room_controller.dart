import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../models/room.dart';
import '../models/player.dart';
import '../network/websocket_client.dart';

final wsClientProvider = Provider((ref) => WebSocketClient());

final roomProvider = StateProvider<Room?>((ref) => null);

final pendingNavigationProvider = StateProvider<String?>((ref) => null);

final roomControllerProvider = Provider((ref) {
  return RoomController(ref);
});

class RoomController {
  final Ref _ref;
  RoomController(this._ref);

  WebSocketClient get _ws => _ref.read(wsClientProvider);

  void listen() {
    final existing = _ws.onMessage;
    _ws.onMessage = (msg) {
      existing?.call(msg);
      _handleMessage(msg);
    };
  }

  void createRoom() {
    _ws.createRoom();
  }

  void joinRoom(String roomId) {
    _ws.joinRoom(roomId);
  }

  void ready(String roomId) {
    _ws.ready(roomId);
  }

  void startGame(String roomId) {
    _ws.startGame(roomId);
  }

  void _handleMessage(Map<String, dynamic> msg) {
    final type = msg['type'] as String?;
    final data = msg['data'] as Map<String, dynamic>? ?? {};

    switch (type) {
      case 'room_created':
        final roomId = data['room_id'] as String;
        _ref.read(roomProvider.notifier).state = Room(
          roomId: roomId,
          isOwner: true,
        );
        _ref.read(pendingNavigationProvider.notifier).state = '/room/$roomId';
        break;

      case 'room_joined':
        final roomId = msg['room_id'] as String? ?? data['room_id'] as String? ?? '';
        final players = _parsePlayers(data);
        _ref.read(roomProvider.notifier).state = Room(
          roomId: roomId,
          players: players,
        );
        _ref.read(pendingNavigationProvider.notifier).state = '/room/$roomId';
        break;

      case 'room_state':
        final room = _ref.read(roomProvider);
        if (room == null) return;
        final players = _parsePlayers(data);
        _ref.read(roomProvider.notifier).state = room.copyWith(players: players);
        break;

      case 'player_joined':
        // room_state 广播会跟随，此处可忽略
        break;

      case 'player_ready':
        final room = _ref.read(roomProvider);
        if (room == null) return;
        final playerId = data['player_id'] as int?;
        if (playerId == null) return;
        final updated = room.players.map((p) {
          if (p.id == playerId) {
            return Player(
              id: p.id,
              nickname: p.nickname,
              seatIndex: p.seatIndex,
              team: p.team,
              isReady: true,
            );
          }
          return p;
        }).toList();
        _ref.read(roomProvider.notifier).state = room.copyWith(players: updated);
        break;

      case 'game_started':
        final room = _ref.read(roomProvider);
        if (room != null) {
          _ref.read(roomProvider.notifier).state =
              room.copyWith(phase: GamePhase.playing);
          _ref.read(pendingNavigationProvider.notifier).state =
              '/game/${room.roomId}';
        }
        break;

      case 'login_result':
        // 登录成功，player_id 可用于后续
        break;
    }
  }

  List<Player> _parsePlayers(Map<String, dynamic> data) {
    final list = data['players'] as List<dynamic>? ?? [];
    return list
        .map((p) => Player.fromJson(p as Map<String, dynamic>))
        .toList();
  }
}
