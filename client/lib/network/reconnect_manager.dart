import 'dart:async';
import 'dart:math';
import 'package:flutter/foundation.dart';
import 'websocket_client.dart';

class ReconnectManager {
  ReconnectManager({required this.client, required this.token}) {
    _prevOnStateChanged = client.onStateChanged;
    client.onStateChanged = _onStateChanged;
  }

  final WebSocketClient client;
  final String token;

  void Function(WsConnectionState)? _prevOnStateChanged;
  Timer? _timer;
  int _attempt = 0;
  bool _disposed = false;
  bool _running = false;

  static const int _maxAttempts = 20;

  void _onStateChanged(WsConnectionState state) {
    _prevOnStateChanged?.call(state);
    if (_disposed) return;

    if (state == WsConnectionState.connected) {
      _attempt = 0;
      _timer?.cancel();
      return;
    }

    if (state == WsConnectionState.disconnected) {
      _scheduleReconnect();
    }
  }

  void _scheduleReconnect() {
    if (_disposed || _running || _attempt >= _maxAttempts) return;

    _timer?.cancel();
    final delay = Duration(seconds: min(30, max(1, pow(2, _attempt).toInt())));
    _attempt++;
    debugPrint('[WS] auto reconnect in ${delay.inSeconds}s (attempt $_attempt)');

    _timer = Timer(delay, () async {
      if (_disposed || _running) return;
      _running = true;
      try {
        await client.connect(token: token);
      } catch (e) {
        debugPrint('[WS] auto reconnect failed: $e');
      } finally {
        _running = false;
      }
    });
  }

  /// 手动重连前调用：停止自动重连计时，重置退避计数。
  void prepareManualReconnect() {
    _timer?.cancel();
    _attempt = 0;
    _running = false;
  }

  void dispose() {
    _disposed = true;
    _timer?.cancel();
    if (_prevOnStateChanged != null) {
      client.onStateChanged = _prevOnStateChanged;
      _prevOnStateChanged = null;
    }
  }
}
