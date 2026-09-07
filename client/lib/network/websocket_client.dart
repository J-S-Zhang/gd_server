import 'dart:async';
import 'dart:convert';
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

  Future<void> connect({required String token}) async {
    _token = token;
    _manualDisconnect = false;
    state = WsConnectionState.connecting;
    onStateChanged?.call(state);

    try {
      final uri = Uri.parse(Constants.wsBaseUrl);
      _channel = WebSocketChannel.connect(uri);
      state = WsConnectionState.connected;
      onStateChanged?.call(state);

      _subscription = _channel!.stream.listen(
        _handleMessage,
        onError: _handleError,
        onDone: _handleDone,
      );

      _startHeartbeat();
      login(token);
    } catch (e) {
      state = WsConnectionState.disconnected;
      onStateChanged?.call(state);
      rethrow;
    }
  }

  void disconnect() {
    _manualDisconnect = true;
    _heartbeatTimer?.cancel();
    _subscription?.cancel();
    _channel?.sink.close();
    state = WsConnectionState.disconnected;
    onStateChanged?.call(state);
  }

  void login(String token) {
    send('login', data: {'token': token});
  }

  void reconnect(String roomId) {
    send('reconnect', roomId: roomId);
  }

  void send(String type, {Map<String, dynamic>? data, String? roomId, int? turnId}) {
    if (_channel == null) return;
    final msg = {
      'protocol_version': Constants.protocolVersion,
      'type': type,
      'request_id': ++_requestId,
      if (roomId != null) 'room_id': roomId,
      if (turnId != null) 'turn_id': turnId,
      'data': data ?? {},
    };
    _channel!.sink.add(jsonEncode(msg));
  }

  void createRoom() => send('create_room');
  void joinRoom(String roomId) => send('join_room', data: {'room_id': roomId}, roomId: roomId);
  void leaveRoom(String roomId) => send('leave_room', roomId: roomId);
  void ready(String roomId) => send('ready', roomId: roomId);
  void startGame(String roomId) => send('start_game', roomId: roomId);
  void playCards(String roomId, List<int> cards, int turnId) =>
      send('play_cards', data: {'cards': cards}, roomId: roomId, turnId: turnId);
  void pass(String roomId, int turnId) => send('pass', roomId: roomId, turnId: turnId);

  void _handleMessage(dynamic raw) {
    try {
      final msg = jsonDecode(raw as String) as Map<String, dynamic>;
      onMessage?.call(msg);
    } catch (_) {}
  }

  void _handleError(Object error) {
    if (_manualDisconnect) return;
    state = WsConnectionState.reconnecting;
    onStateChanged?.call(state);
  }

  void _handleDone() {
    if (_manualDisconnect) return;
    state = WsConnectionState.disconnected;
    onStateChanged?.call(state);
  }

  void _startHeartbeat() {
    _heartbeatTimer?.cancel();
    _heartbeatTimer = Timer.periodic(
      const Duration(seconds: Constants.heartbeatIntervalSeconds),
      (_) => send('ping'),
    );
  }
}
