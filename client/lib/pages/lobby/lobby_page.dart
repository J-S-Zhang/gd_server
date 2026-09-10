import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import '../../config/ui_scale.dart';
import '../../controller/room_controller.dart';
import '../../controller/game_controller.dart';
import '../../controller/auth_controller.dart';
import '../../models/game_mode.dart';
import '../../network/websocket_client.dart';
import '../../network/reconnect_manager.dart';
import '../../theme/game_theme.dart';
import '../../widgets/lobby_user_header.dart';

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
    final ui = context.ui;
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
              _buildHeader(ui, user, wsState),
              Expanded(
                child: Padding(
                  padding: ui.edgeInsetsAll(ui.config.spacing.xxl),
                  child: Center(
                    child: ConstrainedBox(
                      constraints: BoxConstraints(maxWidth: ui.w(ui.config.layout.lobbyMaxWidth)),
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.stretch,
                        children: [
                          if (_connecting)
                            const LinearProgressIndicator(color: GameTheme.accentGold),
                          if (!isConnected && !_connecting) ...[
                            Container(
                              padding: ui.edgeInsetsAll(ui.config.spacing.lg),
                              decoration: GameTheme.panelDecoration(ui),
                              child: Column(
                                children: [
                                  Text(
                                    '无法连接服务器，请确认服务端已启动且端口已放行',
                                    style: TextStyle(
                                      color: Colors.red.shade200,
                                      fontSize: ui.sp(ui.config.font.md),
                                    ),
                                  ),
                                  SizedBox(height: ui.h(ui.config.spacing.md)),
                                  OutlinedButton.icon(
                                    onPressed: _connectWebSocket,
                                    icon: Icon(Icons.refresh, color: Colors.white70, size: ui.sp(ui.config.font.lg)),
                                    label: Text(
                                      '重新连接',
                                      style: TextStyle(color: Colors.white70, fontSize: ui.sp(ui.config.font.md2)),
                                    ),
                                  ),
                                ],
                              ),
                            ),
                            SizedBox(height: ui.h(ui.config.spacing.xl)),
                          ],
                          _lobbyCard(
                            ui,
                            title: '快速开始',
                            child: Column(
                              crossAxisAlignment: CrossAxisAlignment.stretch,
                              children: [
                                Text(
                                  '选择游戏模式',
                                  style: TextStyle(
                                    color: Colors.white70,
                                    fontSize: ui.sp(ui.config.font.md),
                                  ),
                                ),
                                SizedBox(height: ui.h(ui.config.spacing.md + 2)),
                                _buildModeSelector(ui),
                                SizedBox(height: ui.h(ui.config.spacing.xl)),
                                ElevatedButton.icon(
                                  onPressed: isConnected ? _createRoom : null,
                                  icon: Icon(Icons.add, size: ui.sp(ui.config.font.lg)),
                                  label: Text(
                                    _selectedMode == GameMode.solo
                                        ? '创建单人测试房'
                                        : '创建${_selectedMode.label}',
                                    style: TextStyle(fontSize: ui.sp(ui.config.font.xl)),
                                  ),
                                  style: GameTheme.playButtonStyle(ui).copyWith(
                                    minimumSize: WidgetStateProperty.all(
                                      Size(double.infinity, ui.h(ui.config.button.largeHeight)),
                                    ),
                                  ),
                                ),
                                if (_selectedMode == GameMode.solo) ...[
                                  SizedBox(height: ui.h(ui.config.spacing.md)),
                                  Text(
                                    '自动填充 3 名机器人，每人随机发 10 张测试牌',
                                    style: TextStyle(
                                      color: Colors.white.withValues(alpha: 0.45),
                                      fontSize: ui.sp(ui.config.font.sm2),
                                    ),
                                    textAlign: TextAlign.center,
                                  ),
                                ],
                              ],
                            ),
                          ),
                          SizedBox(height: ui.h(ui.config.spacing.xl)),
                          _lobbyCard(
                            ui,
                            title: '加入房间',
                            child: Column(
                              children: [
                                TextField(
                                  controller: _roomIdController,
                                  keyboardType: TextInputType.number,
                                  maxLength: 6,
                                  style: TextStyle(color: Colors.white, fontSize: ui.sp(ui.config.font.md2)),
                                  decoration: InputDecoration(
                                    counterStyle: TextStyle(color: Colors.white38, fontSize: ui.sp(ui.config.font.sm)),
                                    labelText: '输入6位房间号',
                                    labelStyle: TextStyle(color: Colors.white70, fontSize: ui.sp(ui.config.font.md2)),
                                    prefixIcon: Icon(Icons.meeting_room, color: Colors.white70, size: ui.sp(ui.config.font.xl)),
                                    filled: true,
                                    fillColor: Colors.white.withValues(alpha: 0.08),
                                    border: OutlineInputBorder(
                                      borderRadius: BorderRadius.circular(ui.r(ui.config.radius.lg)),
                                    ),
                                  ),
                                ),
                                SizedBox(height: ui.h(ui.config.spacing.lg)),
                                ElevatedButton.icon(
                                  onPressed: isConnected ? _joinRoom : null,
                                  icon: Icon(Icons.login, size: ui.sp(ui.config.font.lg)),
                                  label: Text('加入房间', style: TextStyle(fontSize: ui.sp(ui.config.font.xl))),
                                  style: GameTheme.hintButtonStyle(ui).copyWith(
                                    minimumSize: WidgetStateProperty.all(
                                      Size(double.infinity, ui.h(ui.config.button.largeHeight)),
                                    ),
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

  Widget _buildHeader(UiScale ui, user, WsConnectionState wsState) {
    return Container(
      padding: ui.edgeInsetsSymmetric(horizontal: ui.config.spacing.xl, vertical: ui.config.spacing.md + 2),
      decoration: BoxDecoration(color: Colors.black.withValues(alpha: 0.2)),
      child: Row(
        children: [
          if (user != null) LobbyUserHeader(user: user),
          const Spacer(),
          Text(
            '游戏大厅',
            style: TextStyle(
              color: Colors.white.withValues(alpha: 0.85),
              fontSize: ui.sp(ui.config.font.xl),
              fontWeight: FontWeight.bold,
            ),
          ),
          const Spacer(),
          _buildConnectionIndicator(ui, wsState),
        ],
      ),
    );
  }

  Widget _lobbyCard(UiScale ui, {required String title, required Widget child}) {
    return Container(
      padding: ui.edgeInsetsAll(ui.config.spacing.xl),
      decoration: GameTheme.panelDecoration(ui),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            title,
            style: TextStyle(
              color: GameTheme.accentGold,
              fontSize: ui.sp(ui.config.font.lg),
              fontWeight: FontWeight.bold,
            ),
          ),
          SizedBox(height: ui.h(ui.config.spacing.xl)),
          child,
        ],
      ),
    );
  }

  Widget _buildConnectionIndicator(UiScale ui, WsConnectionState state) {
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
        Icon(Icons.circle, color: color, size: ui.sp(ui.config.font.sm)),
        SizedBox(width: ui.w(ui.config.spacing.sm)),
        Text(label, style: TextStyle(color: color, fontSize: ui.sp(ui.config.font.sm2 + 1))),
      ],
    );
  }

  Widget _buildModeSelector(UiScale ui) {
    return Wrap(
      spacing: ui.w(ui.config.spacing.md),
      runSpacing: ui.h(ui.config.spacing.md),
      children: GameMode.values.map((mode) {
        final selected = _selectedMode == mode;
        return ChoiceChip(
          label: Text(mode.label, style: TextStyle(fontSize: ui.sp(ui.config.font.md2))),
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
