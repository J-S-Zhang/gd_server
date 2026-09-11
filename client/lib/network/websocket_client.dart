import 'dart:async';
import 'dart:convert';
import 'package:flutter/foundation.dart';
import 'package:web_socket_channel/web_socket_channel.dart';
import '../utils/constants.dart';

typedef MessageCallback = void Function(Map<String, dynamic> message);

enum WsConnectionState { disconnected, connecting, connected, reconnecting }

class WebSocketClient {
  WebSocketChannel? _channel;
  StreamSubscription? _subscription;
  Timer? _heartbeatTimer;
  int _requestId = 0;
  String? _token;
  bool _manualDisconnect = false;

  WsConnectionState state = WsConnectionState.disconnected;
  MessageCallback? onMessage;
  void Function(WsConnectionState)? onStateChanged;

  void _setState(WsConnectionState next) {
    if (state == next) return;
    state = next;
    onStateChanged?.call(state);
  }

  Future<void> connect({required String token}) async {
    _token = token;
    _manualDisconnect = false;
    await _cleanupChannel();
    _setState(WsConnectionState.connecting);

    try {
      final uri = Uri.parse(Constants.wsBaseUrl);
      debugPrint('[WS] connecting to $uri');
      _channel = WebSocketChannel.connect(uri);
      await _channel!.ready.timeout(const Duration(seconds: 8));
      debugPrint('[WS] connected');

      _subscription = _channel!.stream.listen(
        _handleMessage,
        onError: _handleError,
        onDone: _handleDone,
        cancelOnError: true,
      );

      _setState(WsConnectionState.connected);
      _startHeartbeat();
      login(token);
    } catch (e) {
      debugPrint('[WS] connect failed: $e');
      await _cleanupChannel();
      _setState(WsConnectionState.disconnected);
      rethrow;
    }
  }

  Future<void> disconnect() async {
    _manualDisconnect = true;
    await _cleanupChannel();
    _setState(WsConnectionState.disconnected);
  }

  void login(String token) {
    send('login', data: {'token': token});
  }

  void reconnect(String roomId) {
    send('reconnect', roomId: roomId);
  }

  bool send(String type, {Map<String, dynamic>? data, String? roomId, int? turnId}) {
    if (_channel == null || state != WsConnectionState.connected) {
      debugPrint('[WS] send skipped ($type): not connected');
      return false;
    }
    final msg = {
      'protocol_version': Constants.protocolVersion,
      'type': type,
      'request_id': ++_requestId,
      if (roomId != null) 'room_id': roomId,
      if (turnId != null) 'turn_id': turnId,
      'data': data ?? {},
    };
    final payload = jsonEncode(msg);
    try {
      _channel!.sink.add(payload);
      debugPrint('[WS] sent: $type (#$_requestId)');
      return true;
    } catch (e) {
      debugPrint('[WS] send failed ($type): $e');
      return false;
    }
  }

  bool createRoom({String mode = 'six'}) =>
      send('create_room', data: {'mode': mode});
  bool joinRoom(String roomId) =>
      send('join_room', data: {'room_id': roomId}, roomId: roomId);
  bool leaveRoom(String roomId) => send('leave_room', roomId: roomId);
  bool ready(String roomId) => send('ready', roomId: roomId);
  bool unready(String roomId) => send('unready', roomId: roomId);
  bool changeSeat(String roomId, int seatIndex) =>
      send('change_seat', data: {'seat_index': seatIndex}, roomId: roomId);
  bool startGame(String roomId) => send('start_game', roomId: roomId);
  bool playCards(String roomId, List<int> cards, int turnId) =>
      send('play_cards', data: {'cards': cards}, roomId: roomId, turnId: turnId);
  bool pass(String roomId, int turnId) => send('pass', roomId: roomId, turnId: turnId);
  bool spectateTeammate(String roomId, int targetSeatIndex) =>
      send('spectate_teammate', data: {'target_seat_index': targetSeatIndex}, roomId: roomId);
  bool requestDismissRoom(String roomId) => send('request_dismiss', roomId: roomId);
  bool voteDismissRoom(String roomId, bool agree) =>
      send('vote_dismiss', data: {'agree': agree}, roomId: roomId);
  bool setRoomOptions(String roomId, {required bool enableTribute}) =>
      send('set_room_options', data: {'enable_tribute': enableTribute}, roomId: roomId);

  void _handleMessage(dynamic raw) {
    try {
      final text = raw is String ? raw : raw.toString();
      debugPrint('[WS] recv: ${text.length > 200 ? '${text.substring(0, 200)}...' : text}');
      final msg = jsonDecode(text) as Map<String, dynamic>;
      onMessage?.call(msg);
    } catch (e) {
      debugPrint('[WS] parse error: $e');
    }
  }

  void _handleError(Object error) {
    debugPrint('[WS] stream error: $error');
    if (_manualDisconnect) return;
    _setState(WsConnectionState.reconnecting);
  }

  void _handleDone() {
    debugPrint('[WS] stream closed');
    if (_manualDisconnect) return;
    _setState(WsConnectionState.disconnected);
  }

  Future<void> _cleanupChannel() async {
    _heartbeatTimer?.cancel();
    _heartbeatTimer = null;
    await _subscription?.cancel();
    _subscription = null;
    try {
      await _channel?.sink.close();
    } catch (_) {}
    _channel = null;
  }

  void _startHeartbeat() {
    _heartbeatTimer?.cancel();
    _heartbeatTimer = Timer.periodic(
      const Duration(seconds: Constants.heartbeatIntervalSeconds),
      (_) => send('ping'),
    );
  }
}
