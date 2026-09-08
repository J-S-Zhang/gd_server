import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import '../../controller/room_controller.dart';
import '../../controller/game_controller.dart';
import '../../controller/auth_controller.dart';
import '../../models/game_mode.dart';
import '../../network/websocket_client.dart';
import '../../network/reconnect_manager.dart';
import '../../theme/game_theme.dart';

class LobbyPage extends ConsumerStatefulWidget {
  const LobbyPage({super.key});

  @override
  ConsumerState<LobbyPage> createState() => _LobbyPageState();
}

class _LobbyPageState extends ConsumerState<LobbyPage> {
  final _roomIdController = TextEditingController();
  bool _connecting = false;
  GameMode _selectedMode = GameMode.six;
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
      ref.read(gameControllerProvider).listen();
      await ws.connect(token: user.token);
      _reconnectManager?.dispose();
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
    final wsState = ref.watch(wsConnectionStateProvider);
    final isConnected = wsState == WsConnectionState.connected;

    ref.listen<String?>(wsErrorProvider, (prev, next) {
      if (next != null && mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text(next)),
        );
        ref.read(wsErrorProvider.notifier).state = null;
      }
    });

    return Scaffold(
      body: Container(
        decoration: const BoxDecoration(gradient: GameTheme.pageGradient),
        child: SafeArea(
          child: Column(
            children: [
              _buildHeader(user, wsState),
              Expanded(
                child: Padding(
                  padding: const EdgeInsets.all(24),
                  child: Center(
                    child: ConstrainedBox(
                      constraints: const BoxConstraints(maxWidth: 520),
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.stretch,
                        children: [
                          if (_connecting) const LinearProgressIndicator(color: GameTheme.accentGold),
                          if (!isConnected && !_connecting) ...[
                            Container(
                              padding: const EdgeInsets.all(12),
                              decoration: GameTheme.panelDecoration(),
                              child: Column(
                                children: [
                                  Text(
                                    '无法连接服务器，请确认服务端已启动且端口已放行',
                                    style: TextStyle(color: Colors.red.shade200, fontSize: 13),
                                  ),
                                  const SizedBox(height: 8),
                                  OutlinedButton.icon(
                                    onPressed: _connectWebSocket,
                                    icon: const Icon(Icons.refresh, color: Colors.white70),
                                    label: const Text('重新连接', style: TextStyle(color: Colors.white70)),
                                  ),
                                ],
                              ),
                            ),
                            const SizedBox(height: 16),
                          ],
                          _lobbyCard(
                            title: '快速开始',
                            child: Column(
                              crossAxisAlignment: CrossAxisAlignment.stretch,
                              children: [
                                const Text(
                                  '选择游戏模式',
                                  style: TextStyle(color: Colors.white70, fontSize: 13),
                                ),
                                const SizedBox(height: 10),
                                _buildModeSelector(),
                                const SizedBox(height: 16),
                                ElevatedButton.icon(
                                  onPressed: isConnected ? _createRoom : null,
                                  icon: const Icon(Icons.add),
                                  label: Text(
                                    _selectedMode == GameMode.solo
                                        ? '创建单人测试房'
                                        : '创建${_selectedMode.label}',
                                    style: const TextStyle(fontSize: 18),
                                  ),
                                  style: GameTheme.playButtonStyle.copyWith(
                                    minimumSize: WidgetStateProperty.all(
                                      const Size(double.infinity, 52),
                                    ),
                                  ),
                                ),
                                if (_selectedMode == GameMode.solo) ...[
                                  const SizedBox(height: 8),
                                  Text(
                                    '自动填充 3 名机器人，便于开发调试',
                                    style: TextStyle(
                                      color: Colors.white.withValues(alpha: 0.45),
                                      fontSize: 11,
                                    ),
                                    textAlign: TextAlign.center,
                                  ),
                                ],
                              ],
                            ),
                          ),
                          const SizedBox(height: 20),
                          _lobbyCard(
                            title: '加入房间',
                            child: Column(
                              children: [
                                TextField(
                                  controller: _roomIdController,
                                  keyboardType: TextInputType.number,
                                  maxLength: 6,
                                  style: const TextStyle(color: Colors.white),
                                  decoration: InputDecoration(
                                    counterStyle: const TextStyle(color: Colors.white38),
                                    labelText: '输入6位房间号',
                                    labelStyle: const TextStyle(color: Colors.white70),
                                    prefixIcon: const Icon(Icons.meeting_room, color: Colors.white70),
                                    filled: true,
                                    fillColor: Colors.white.withValues(alpha: 0.08),
                                    border: OutlineInputBorder(
                                      borderRadius: BorderRadius.circular(12),
                                    ),
                                  ),
                                ),
                                const SizedBox(height: 12),
                                ElevatedButton.icon(
                                  onPressed: isConnected ? _joinRoom : null,
                                  icon: const Icon(Icons.login),
                                  label: const Text('加入房间', style: TextStyle(fontSize: 18)),
                                  style: GameTheme.hintButtonStyle.copyWith(
                                    minimumSize: WidgetStateProperty.all(const Size(double.infinity, 52)),
                                  ),
                                ),
                              ],
                            ),
                          ),
                        ],
                      ),
                    ),
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildHeader(user, WsConnectionState wsState) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 10),
      decoration: BoxDecoration(color: Colors.black.withValues(alpha: 0.2)),
      child: Row(
        children: [
          const Text(
            '游戏大厅',
            style: TextStyle(color: Colors.white, fontSize: 20, fontWeight: FontWeight.bold),
          ),
          const Spacer(),
          _buildConnectionIndicator(wsState),
          if (user != null) ...[
            const SizedBox(width: 12),
            CircleAvatar(
              radius: 16,
              backgroundColor: GameTheme.tableBlueLight,
              child: Text(user.nickname[0], style: const TextStyle(color: Colors.white)),
            ),
            const SizedBox(width: 8),
            Text(user.nickname, style: const TextStyle(color: Colors.white70)),
          ],
        ],
      ),
    );
  }

  Widget _lobbyCard({required String title, required Widget child}) {
    return Container(
      padding: const EdgeInsets.all(20),
      decoration: GameTheme.panelDecoration(),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            title,
            style: const TextStyle(
              color: GameTheme.accentGold,
              fontSize: 16,
              fontWeight: FontWeight.bold,
            ),
          ),
          const SizedBox(height: 16),
          child,
        ],
      ),
    );
  }

  Widget _buildConnectionIndicator(WsConnectionState state) {
    Color color;
    String label;
    switch (state) {
      case WsConnectionState.connected:
        color = Colors.greenAccent;
        label = '在线';
        break;
      case WsConnectionState.connecting:
      case WsConnectionState.reconnecting:
        color = Colors.orange;
        label = '连接中';
        break;
      default:
        color = Colors.redAccent;
        label = '离线';
    }
    return Row(
      children: [
        Icon(Icons.circle, color: color, size: 10),
        const SizedBox(width: 4),
        Text(label, style: TextStyle(color: color, fontSize: 12)),
      ],
    );
  }

  Widget _buildModeSelector() {
    return Wrap(
      spacing: 8,
      runSpacing: 8,
      children: GameMode.values.map((mode) {
        final selected = _selectedMode == mode;
        return ChoiceChip(
          label: Text(mode.label),
          selected: selected,
          onSelected: (_) => setState(() => _selectedMode = mode),
          selectedColor: GameTheme.accentGold.withValues(alpha: 0.35),
          backgroundColor: Colors.white.withValues(alpha: 0.08),
          labelStyle: TextStyle(
            color: selected ? Colors.white : Colors.white70,
            fontWeight: selected ? FontWeight.bold : FontWeight.normal,
          ),
          side: BorderSide(
            color: selected ? GameTheme.accentGold : Colors.white24,
          ),
        );
      }).toList(),
    );
  }

  void _createRoom() {
    final sent = ref.read(roomControllerProvider).createRoom(_selectedMode);
    if (sent && mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('正在创建房间...')),
      );
    }
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
