import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../config/ui_scale.dart';
import '../../controller/room_controller.dart';
import '../../controller/game_controller.dart';
import '../../controller/auth_controller.dart';
import '../../models/game_mode.dart';
import '../../network/websocket_client.dart';
import '../../network/reconnect_manager.dart';
import '../../theme/game_theme.dart';
import '../../widgets/game/game_layout_positioned.dart';
import '../../utils/region_layout.dart';
import '../../widgets/game/region_child_stack.dart';
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
    ref.read(roomControllerProvider).listen();
    ref.read(gameControllerProvider).listen();
    _connectWebSocket();
  }

  Future<void> _connectWebSocket() async {
    final user = ref.read(userProvider);
    if (user == null) return;

    _reconnectManager?.prepareManualReconnect();
    _reconnectManager?.dispose();
    _reconnectManager = null;

    setState(() => _connecting = true);
    try {
      final ws = ref.read(wsClientProvider);
      await ws.disconnect();
      await ws.connect(token: user.token);
      _reconnectManager = ReconnectManager(client: ws, token: user.token);
    } catch (_) {
      // 连接失败时由 connection_error 区域的重新连接按钮提示即可。
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
          child: Stack(
            fit: StackFit.expand,
            clipBehavior: Clip.hardEdge,
            children: [
              PageLayoutPositioned(
                page: PageLayoutKind.lobby,
                elementId: 'header',
                alignment: Alignment.center,
                child: _buildHeader(ui, user, wsState),
              ),
              if (_connecting)
                PageLayoutPositioned(
                  page: PageLayoutKind.lobby,
                  elementId: 'header',
                  alignment: Alignment.bottomCenter,
                  child: const LinearProgressIndicator(color: GameTheme.accentGold),
                ),
              if (!isConnected && !_connecting)
                PageLayoutPositioned(
                  page: PageLayoutKind.lobby,
                  elementId: 'connection_error',
                  child: _buildConnectionError(ui),
                ),
              PageLayoutPositioned(
                page: PageLayoutKind.lobby,
                elementId: 'quick_start',
                child: _buildQuickStart(ui, isConnected),
              ),
              PageLayoutPositioned(
                page: PageLayoutKind.lobby,
                elementId: 'join_room',
                child: _buildJoinRoom(ui, isConnected),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildHeader(UiScale ui, user, WsConnectionState wsState) {
    const parentId = 'header';
    const page = PageLayoutKind.lobby;
    final region = ui.layoutRect(page, parentId);
    if (region == null) {
      return const SizedBox.shrink();
    }

    return SizedBox(
      width: region.width,
      height: region.height,
      child: DecoratedBox(
        decoration: BoxDecoration(color: Colors.black.withValues(alpha: 0.2)),
        child: Stack(
          clipBehavior: Clip.none,
          children: [
            if (user != null)
              RegionChildPositioned(
                page: page,
                parentId: parentId,
                childId: 'user_profile',
                child: LobbyUserHeader(
                  user: user,
                  regionHeight: ui.elementChildRect(parentId, 'user_profile', page: page)?.height,
                ),
              ),
            RegionChildPositioned(
              page: page,
              parentId: parentId,
              childId: 'title',
              child: _buildChildText(
                ui,
                parentId: parentId,
                childId: 'title',
                page: page,
                text: '游戏大厅',
                fontFactor: 0.42,
                fallbackFont: ui.config.font.xl,
                fontWeight: FontWeight.bold,
                color: Colors.white.withValues(alpha: 0.85),
              ),
            ),
            RegionChildPositioned(
              page: page,
              parentId: parentId,
              childId: 'connection_status',
              child: Center(
                child: _buildConnectionIndicator(
                  ui,
                  wsState,
                  ui.elementChildRect(parentId, 'connection_status', page: page),
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildConnectionError(UiScale ui) {
    final region = ui.layoutRect(PageLayoutKind.lobby, 'connection_error');
    final pad = region != null ? region.width * 0.04 : ui.w(ui.config.spacing.lg);
    final fontSize = region != null ? region.height * 0.22 : ui.sp(ui.config.font.md);

    return Container(
      padding: EdgeInsets.all(pad),
      decoration: GameTheme.panelDecoration(ui, radius: region?.height != null ? region!.height * 0.12 : null),
      child: Center(
        child: OutlinedButton.icon(
          onPressed: _connectWebSocket,
          icon: Icon(Icons.refresh, color: Colors.white70, size: fontSize * 1.2),
          label: Text('重新连接', style: TextStyle(color: Colors.white70, fontSize: fontSize)),
        ),
      ),
    );
  }

  Widget _buildQuickStart(UiScale ui, bool isConnected) {
    const page = PageLayoutKind.lobby;
    const parentId = 'quick_start';
    final region = ui.layoutRect(page, parentId);
    if (region == null) {
      return const SizedBox.shrink();
    }

    final btnRect = ui.elementChildRect(parentId, 'create_button', page: page);
    final btnSize = btnRect != null
        ? Size(btnRect.width, btnRect.height)
        : Size(region.width * 0.92, region.height * 0.18);
    final btnFont = btnSize.height * 0.38;

    return SizedBox(
      width: region.width,
      height: region.height,
      child: DecoratedBox(
        decoration: GameTheme.panelDecoration(ui, radius: region.height * 0.05),
        child: Stack(
          clipBehavior: Clip.none,
          children: [
            RegionChildPositioned(
              page: page,
              parentId: parentId,
              childId: 'section_title',
              child: _buildChildText(
                ui,
                parentId: parentId,
                childId: 'section_title',
                page: page,
                text: '快速开始',
                fontFactor: 0.55,
                fallbackFont: ui.config.font.lg,
                fontWeight: FontWeight.bold,
                color: GameTheme.accentGold,
              ),
            ),
            RegionChildPositioned(
              page: page,
              parentId: parentId,
              childId: 'mode_selector',
              child: _buildModeSelector(ui, parentId: parentId, page: page),
            ),
            RegionChildPositioned(
              page: page,
              parentId: parentId,
              childId: 'create_button',
              child: ElevatedButton.icon(
                onPressed: isConnected ? _createRoom : null,
                icon: Icon(Icons.add, size: btnFont),
                label: FittedBox(
                  fit: BoxFit.scaleDown,
                  child: Text(
                    _selectedMode == GameMode.solo
                        ? '创建单人测试房'
                        : '创建${_selectedMode.label}',
                    style: TextStyle(fontSize: btnFont),
                  ),
                ),
                style: GameTheme.playButtonStyle(ui, minSize: btnSize),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildJoinRoom(UiScale ui, bool isConnected) {
    const page = PageLayoutKind.lobby;
    const parentId = 'join_room';
    final region = ui.layoutRect(page, parentId);
    if (region == null) {
      return const SizedBox.shrink();
    }

    final inputRect = ui.elementChildRect(parentId, 'room_input', page: page);
    final btnRect = ui.elementChildRect(parentId, 'join_button', page: page);
    final fieldFont = inputRect != null
        ? inputRect.height * 0.28
        : ui.sp(ui.config.font.md2);
    final btnSize = btnRect != null
        ? Size(btnRect.width, btnRect.height)
        : Size(region.width * 0.92, region.height * 0.22);
    final btnFont = btnSize.height * 0.38;

    return SizedBox(
      width: region.width,
      height: region.height,
      child: DecoratedBox(
        decoration: GameTheme.panelDecoration(ui, radius: region.height * 0.05),
        child: Stack(
          clipBehavior: Clip.none,
          children: [
            RegionChildPositioned(
              page: page,
              parentId: parentId,
              childId: 'section_title',
              child: _buildChildText(
                ui,
                parentId: parentId,
                childId: 'section_title',
                page: page,
                text: '加入房间',
                fontFactor: 0.55,
                fallbackFont: ui.config.font.lg,
                fontWeight: FontWeight.bold,
                color: GameTheme.accentGold,
              ),
            ),
            RegionChildPositioned(
              page: page,
              parentId: parentId,
              childId: 'room_input',
              child: TextField(
                controller: _roomIdController,
                keyboardType: TextInputType.number,
                maxLength: 6,
                maxLines: 1,
                style: TextStyle(color: Colors.white, fontSize: fieldFont),
                decoration: InputDecoration(
                  counterStyle: TextStyle(color: Colors.white38, fontSize: fieldFont * 0.7),
                  labelText: '输入6位房间号',
                  labelStyle: TextStyle(color: Colors.white70, fontSize: fieldFont),
                  prefixIcon: Icon(Icons.meeting_room, color: Colors.white70, size: fieldFont * 1.2),
                  filled: true,
                  fillColor: Colors.white.withValues(alpha: 0.08),
                  border: OutlineInputBorder(
                    borderRadius: BorderRadius.circular(
                      (inputRect?.height ?? region.height * 0.5) * 0.12,
                    ),
                  ),
                ),
              ),
            ),
            RegionChildPositioned(
              page: page,
              parentId: parentId,
              childId: 'join_button',
              child: ElevatedButton.icon(
                onPressed: isConnected ? _joinRoom : null,
                icon: Icon(Icons.login, size: btnFont),
                label: Text('加入房间', style: TextStyle(fontSize: btnFont)),
                style: GameTheme.hintButtonStyle(ui, minSize: btnSize),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildChildText(
    UiScale ui, {
    required String parentId,
    required String childId,
    required PageLayoutKind page,
    required String text,
    required double fontFactor,
    required double fallbackFont,
    FontWeight fontWeight = FontWeight.normal,
    required Color color,
  }) {
    final rect = ui.elementChildRect(parentId, childId, page: page);
    final fontSize = rect != null ? rect.height * fontFactor : ui.sp(fallbackFont);
    return Center(
      child: Text(
        text,
        style: TextStyle(color: color, fontSize: fontSize, fontWeight: fontWeight),
        textAlign: TextAlign.center,
      ),
    );
  }

  Widget _buildConnectionIndicator(UiScale ui, WsConnectionState state, RegionChildRect? region) {
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
    final fontSize = region != null ? region.height * 0.28 : ui.sp(ui.config.font.sm2 + 1);
    return Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        Icon(Icons.circle, color: color, size: fontSize * 0.8),
        SizedBox(width: region != null ? region.width * 0.01 : ui.w(ui.config.spacing.sm)),
        Text(label, style: TextStyle(color: color, fontSize: fontSize)),
      ],
    );
  }

  Widget _buildModeSelector(
    UiScale ui, {
    required String parentId,
    required PageLayoutKind page,
  }) {
    final rect = ui.elementChildRect(parentId, 'mode_selector', page: page);
    final spacing = rect != null ? rect.width * 0.02 : ui.w(ui.config.spacing.md);
    final chipFont = rect != null ? rect.height * 0.22 : ui.sp(ui.config.font.md2);

    return Align(
      alignment: Alignment.center,
      child: Wrap(
        spacing: spacing,
        runSpacing: spacing,
        alignment: WrapAlignment.center,
        children: GameMode.values.map((mode) {
        final selected = _selectedMode == mode;
        return ChoiceChip(
          label: Text(mode.label, style: TextStyle(fontSize: chipFont)),
          selected: selected,
          onSelected: (_) => setState(() => _selectedMode = mode),
          selectedColor: GameTheme.accentGold.withValues(alpha: 0.35),
          backgroundColor: Colors.white.withValues(alpha: 0.08),
          labelStyle: TextStyle(
            color: selected ? Colors.white : Colors.white70,
            fontWeight: selected ? FontWeight.bold : FontWeight.normal,
          ),
          side: BorderSide(color: selected ? GameTheme.accentGold : Colors.white24),
        );
      }).toList(),
      ),
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
