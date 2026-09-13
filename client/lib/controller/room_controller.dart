import 'package:flutter/foundation.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../models/dismiss_vote.dart';
import '../models/room.dart';
import '../models/player.dart';
import '../models/game_mode.dart';
import '../network/websocket_client.dart';
import 'auth_controller.dart';

final wsConnectionStateProvider =
    StateProvider<WsConnectionState>((ref) => WsConnectionState.disconnected);

final wsClientProvider = Provider((ref) {
  final client = WebSocketClient();
  client.onStateChanged = (state) {
    ref.read(wsConnectionStateProvider.notifier).state = state;
  };
  ref.onDispose(() {
    client.disconnect();
  });
  return client;
});

final roomProvider = StateProvider<Room?>((ref) => null);

final pendingNavigationProvider = StateProvider<String?>((ref) => null);

final dismissVoteProvider = StateProvider<DismissVoteState?>((ref) => null);

final wsErrorProvider = StateProvider<String?>((ref) => null);

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

  bool createRoom([GameMode mode = GameMode.six]) {
    _ref.read(wsErrorProvider.notifier).state = null;
    if (!_ws.createRoom(mode: mode.wireValue)) {
      _ref.read(wsErrorProvider.notifier).state = '未连接到服务器，请检查网络或稍后重试';
      return false;
    }
    return true;
  }

  bool joinRoom(String roomId) {
    _ref.read(wsErrorProvider.notifier).state = null;
    if (!_ws.joinRoom(roomId)) {
      _ref.read(wsErrorProvider.notifier).state = '未连接到服务器，请检查网络或稍后重试';
      return false;
    }
    return true;
  }

  void ready(String roomId) {
    _ws.ready(roomId);
  }

  void unready(String roomId) {
    _ws.unready(roomId);
  }

  void changeSeat(String roomId, int seatIndex) {
    _ws.changeSeat(roomId, seatIndex);
  }

  void startGame(String roomId) {
    _ws.startGame(roomId);
  }

  void leaveRoom(String roomId) {
    _ws.leaveRoom(roomId);
    _ref.read(roomProvider.notifier).state = null;
    _ref.read(dismissVoteProvider.notifier).state = null;
    _ref.read(pendingNavigationProvider.notifier).state = null;
  }

  bool requestDismissRoom(String roomId) {
    _ref.read(wsErrorProvider.notifier).state = null;
    if (!_ws.requestDismissRoom(roomId)) {
      _ref.read(wsErrorProvider.notifier).state = '未连接到服务器，无法申请解散';
      return false;
    }
    return true;
  }

  void voteDismissRoom(String roomId, bool agree) {
    _ws.voteDismissRoom(roomId, agree);
  }

  bool setEnableTribute(String roomId, bool enabled) {
    _ref.read(wsErrorProvider.notifier).state = null;
    if (!_ws.setRoomOptions(roomId, enableTribute: enabled)) {
      _ref.read(wsErrorProvider.notifier).state = '未连接到服务器，无法修改房间设置';
      return false;
    }
    return true;
  }

  void _handleMessage(Map<String, dynamic> msg) {
    final type = msg['type'] as String?;
    final data = msg['data'] as Map<String, dynamic>? ?? {};

    switch (type) {
      case 'room_created':
        final roomId = data['room_id']?.toString();
        if (roomId == null || roomId.isEmpty) {
          _ref.read(wsErrorProvider.notifier).state = '创建房间失败：服务器返回无效房间号';
          return;
        }
        _ref.read(roomProvider.notifier).state = _roomFromData(
          roomId,
          data,
          isOwner: true,
        );
        _ref.read(pendingNavigationProvider.notifier).state = '/game/$roomId';
        break;

      case 'room_joined':
        final roomId = msg['room_id']?.toString() ??
            data['room_id']?.toString() ??
            '';
        _ref.read(roomProvider.notifier).state = _roomFromData(roomId, data);
        _ref.read(pendingNavigationProvider.notifier).state = '/game/$roomId';
        break;

      case 'room_state':
        final roomId = msg['room_id']?.toString() ?? '';
        final room = _ref.read(roomProvider);
        if (room != null) {
          _ref.read(roomProvider.notifier).state = _mergeRoomData(room, data);
        } else if (roomId.isNotEmpty) {
          final userId = _ref.read(userProvider)?.id;
          final players = _parsePlayers(data);
          final isOwner = userId != null &&
              players.any((p) => p.id == userId && _playerIsOwner(p, data, userId));
          _ref.read(roomProvider.notifier).state = _roomFromData(
            roomId,
            data,
            isOwner: isOwner,
          );
        }
        break;

      case 'player_joined':
        // room_state 广播会跟随，此处可忽略
        break;

      case 'player_ready':
      case 'player_unready':
      case 'seat_changed':
      case 'room_options_updated':
        // room_state 广播会跟随，此处可忽略
        break;

      case 'game_started':
        final room = _ref.read(roomProvider);
        final roomId = msg['room_id']?.toString() ??
            data['room_id']?.toString() ??
            room?.roomId;
        if (roomId == null || roomId.isEmpty) return;
        if (room != null) {
          _ref.read(roomProvider.notifier).state =
              room.copyWith(phase: GamePhase.playing);
        } else {
          _ref.read(roomProvider.notifier).state = Room(
            roomId: roomId,
            phase: GamePhase.playing,
          );
        }
        _ref.read(pendingNavigationProvider.notifier).state = '/game/$roomId';
        break;

      case 'login_result':
        _handleLoginResult(msg['data'] as Map<String, dynamic>? ?? {});
        break;

      case 'dismiss_vote_started':
      case 'dismiss_vote_updated':
        _ref.read(dismissVoteProvider.notifier).state =
            DismissVoteState.fromJson(data);
        break;

      case 'dismiss_vote_rejected':
        _ref.read(dismissVoteProvider.notifier).state = null;
        _ref.read(wsErrorProvider.notifier).state = '解散申请被拒绝';
        break;

      case 'room_dismissed':
        _ref.read(dismissVoteProvider.notifier).state = null;
        _ref.read(roomProvider.notifier).state = null;
        _ref.read(pendingNavigationProvider.notifier).state = '/lobby';
        break;

      case 'error':
        final code = msg['error_code'];
        final codeStr = code?.toString() ?? '';
        if (code == 2003 || codeStr == '2003') {
          _ref.read(wsErrorProvider.notifier).state =
              '您已在房间中，正在为您自动进入...';
          return;
        }
        if (code == 4002 || codeStr == '4002') {
          final reqType = _ws.requestTypeFor(msg['request_id']);
          if (reqType == 'seat_chat' ||
              reqType == 'voice_state' ||
              reqType == 'voice_signal') {
            debugPrint('[WS] ignored $reqType unsupported on server (4002)');
            return;
          }
        }
        _ref.read(wsErrorProvider.notifier).state =
            '操作失败${codeStr.isNotEmpty ? ' (错误码: $codeStr)' : ''}';
        break;
    }
  }

  void _handleLoginResult(Map<String, dynamic> data) {
    if (data['success'] != true) return;
    if (data['in_room'] != true) return;

    final roomId = data['room_id']?.toString();
    if (roomId == null || roomId.isEmpty) return;

    final phase = data['room_phase']?.toString() ?? 'WAITING';
    final isOwner = data['is_owner'] == true;

    final mode = GameMode.fromString(data['mode'] as String?);
    _ref.read(roomProvider.notifier).state = Room(
      roomId: roomId,
      isOwner: isOwner,
      phase: _parseRoomPhase(phase),
      mode: mode,
      maxPlayers: mode.maxPlayers,
    );

    if (phase == 'PLAYING' || phase == 'SETTLEMENT') {
      _ref.read(pendingNavigationProvider.notifier).state = '/game/$roomId';
    } else {
      _ref.read(pendingNavigationProvider.notifier).state = '/game/$roomId';
    }
  }

  GamePhase _parseRoomPhase(String phase) {
    switch (phase.toUpperCase()) {
      case 'PLAYING':
        return GamePhase.playing;
      case 'SETTLEMENT':
        return GamePhase.settlement;
      default:
        return GamePhase.waiting;
    }
  }

  bool _playerIsOwner(Player player, Map<String, dynamic> data, int userId) {
    if (player.id != userId) return false;
    final list = data['players'] as List<dynamic>? ?? [];
    for (final raw in list) {
      final p = raw as Map<String, dynamic>;
      if (p['id'] == userId) {
        return p['is_owner'] == true;
      }
    }
    return false;
  }

  Room _roomFromData(
    String roomId,
    Map<String, dynamic> data, {
    bool isOwner = false,
    GamePhase phase = GamePhase.waiting,
  }) {
    return Room.fromRoomData(
      roomId: roomId,
      data: data,
      isOwner: isOwner,
      phase: phase,
    );
  }

  Room _mergeRoomData(Room room, Map<String, dynamic> data) {
    final mode = data.containsKey('mode')
        ? GameMode.fromString(data['mode'] as String?)
        : room.mode;
    return room.copyWith(
      players: _parsePlayers(data),
      mode: mode,
      maxPlayers: mode.maxPlayers,
      enableTribute: data.containsKey('enable_tribute')
          ? data['enable_tribute'] == true
          : room.enableTribute,
    );
  }

  List<Player> _parsePlayers(Map<String, dynamic> data) {
    final list = data['players'] as List<dynamic>? ?? [];
    return list
        .map((p) => Player.fromJson(p as Map<String, dynamic>))
        .toList();
  }
}
