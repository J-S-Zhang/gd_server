import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import '../../controller/room_controller.dart';
import '../../controller/auth_controller.dart';
import '../../network/websocket_client.dart';
import '../../network/reconnect_manager.dart';

class LobbyPage extends ConsumerStatefulWidget {
  const LobbyPage({super.key});

  @override
  ConsumerState<LobbyPage> createState() => _LobbyPageState();
}

class _LobbyPageState extends ConsumerState<LobbyPage> {
  final _roomIdController = TextEditingController();
  bool _connecting = false;
  ReconnectManager? _reconnectManager;

  @override
  void initState() {
    super.initState();
    _connectWebSocket();
  }

  Future<void> _connectWebSocket() async {
    final user = ref.read(userProvider);
    if (user == null) return;
    setState(() => _connecting = true);
    try {
      final ws = ref.read(wsClientProvider);
      ref.read(roomControllerProvider).listen();
      await ws.connect(token: user.token);
      _reconnectManager = ReconnectManager(client: ws, token: user.token);
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('连接服务器失败: $e')),
        );
      }
    } finally {
      if (mounted) setState(() => _connecting = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    final user = ref.watch(userProvider);
    final wsState = ref.watch(wsClientProvider).state;

    ref.listen<String?>(pendingNavigationProvider, (prev, next) {
      if (next != null && mounted) {
        ref.read(pendingNavigationProvider.notifier).state = null;
        context.go(next);
      }
    });

    return Scaffold(
      appBar: AppBar(
        title: const Text('游戏大厅'),
        actions: [
          _buildConnectionIndicator(wsState),
          if (user != null)
            Padding(
              padding: const EdgeInsets.only(right: 16),
              child: Center(child: Text(user.nickname)),
            ),
        ],
      ),
      body: Padding(
        padding: const EdgeInsets.all(24),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            if (_connecting) const LinearProgressIndicator(),
            const SizedBox(height: 32),
            ElevatedButton.icon(
              onPressed: wsState == ConnectionState.connected ? _createRoom : null,
              icon: const Icon(Icons.add),
              label: const Text('创建房间', style: TextStyle(fontSize: 18)),
              style: ElevatedButton.styleFrom(
                padding: const EdgeInsets.all(20),
                backgroundColor: Colors.green,
              ),
            ),
            const SizedBox(height: 32),
            const Divider(),
            const SizedBox(height: 16),
            TextField(
              controller: _roomIdController,
              keyboardType: TextInputType.number,
              maxLength: 6,
              decoration: const InputDecoration(
                labelText: '输入房间号',
                prefixIcon: Icon(Icons.meeting_room),
                border: OutlineInputBorder(),
              ),
            ),
            const SizedBox(height: 16),
            ElevatedButton.icon(
              onPressed: wsState == ConnectionState.connected ? _joinRoom : null,
              icon: const Icon(Icons.login),
              label: const Text('加入房间', style: TextStyle(fontSize: 18)),
              style: ElevatedButton.styleFrom(
                padding: const EdgeInsets.all(20),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildConnectionIndicator(ConnectionState state) {
    Color color;
    String label;
    switch (state) {
      case ConnectionState.connected:
        color = Colors.green;
        label = '在线';
        break;
      case ConnectionState.connecting:
      case ConnectionState.reconnecting:
        color = Colors.orange;
        label = '连接中';
        break;
      default:
        color = Colors.red;
        label = '离线';
    }
    return Padding(
      padding: const EdgeInsets.only(right: 8),
      child: Row(
        children: [
          Icon(Icons.circle, color: color, size: 12),
          const SizedBox(width: 4),
          Text(label, style: TextStyle(color: color, fontSize: 12)),
        ],
      ),
    );
  }

  void _createRoom() {
    ref.read(roomControllerProvider).createRoom();
  }

  void _joinRoom() {
    final roomId = _roomIdController.text.trim();
    if (roomId.length != 6) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('请输入6位房间号')),
      );
      return;
    }
    ref.read(roomControllerProvider).joinRoom(roomId);
  }

  @override
  void dispose() {
    _reconnectManager?.dispose();
    _roomIdController.dispose();
    super.dispose();
  }
}
