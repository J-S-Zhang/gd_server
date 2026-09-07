import 'dart:async';
import 'dart:math';
import 'websocket_client.dart';

class ReconnectManager {
  final WebSocketClient client;
  final String token;
  Timer? _timer;
  int _attempt = 0;
  static const int _maxAttempts = 10;

  ReconnectManager({required this.client, required this.token}) {
    final prev = client.onStateChanged;
    client.onStateChanged = (state) {
      prev?.call(state);
      _onStateChanged(state);
    };
  }

  void _onStateChanged(WsConnectionState state) {
    if (state == WsConnectionState.disconnected ||
        state == WsConnectionState.reconnecting) {
      _scheduleReconnect();
    } else if (state == WsConnectionState.connected) {
      _attempt = 0;
      _timer?.cancel();
    }
  }

  void _scheduleReconnect() {
    if (_attempt >= _maxAttempts) return;
    _timer?.cancel();
    final delay = Duration(seconds: min(30, pow(2, _attempt).toInt()));
    _attempt++;
    _timer = Timer(delay, () async {
      try {
        await client.connect(token: token);
      } catch (_) {}
    });
  }

  void dispose() {
    _timer?.cancel();
  }
}
