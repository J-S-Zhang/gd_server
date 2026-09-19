import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import '../../config/ui_scale.dart';
import '../../controller/game_controller.dart';
import '../../controller/room_controller.dart';
import '../../models/game_state.dart';
import '../../theme/game_theme.dart';

class ResultPage extends ConsumerWidget {
  const ResultPage({super.key});

  Future<void> _returnToLobby(WidgetRef ref, BuildContext context) async {
    final roomId = ref.read(roomProvider)?.roomId;
    if (roomId != null && roomId.isNotEmpty) {
      await ref.read(roomControllerProvider).leaveRoom(roomId);
    }
    ref.read(gameStateProvider.notifier).state = const ClientGameState();
    if (context.mounted) context.go('/lobby');
  }

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final ui = context.ui;

    return Scaffold(
      body: Container(
        decoration: const BoxDecoration(gradient: GameTheme.pageGradient),
        child: Center(
          child: Container(
              margin: ui.edgeInsetsAll(32),
              padding: ui.edgeInsetsAll(32),
              decoration: GameTheme.panelDecoration(ui),
              child: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Icon(
                    Icons.emoji_events,
                    size: ui.sp(80),
                    color: GameTheme.accentGold,
                  ),
                  SizedBox(height: ui.h(24)),
                  Text(
                    '本局结束',
                    style: TextStyle(
                      fontSize: ui.sp(32),
                      color: Colors.white,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                  SizedBox(height: ui.h(12)),
                  Text(
                    '感谢参与，期待下一局',
                    style: TextStyle(
                      color: GameTheme.textSecondary,
                      fontSize: ui.sp(14),
                    ),
                  ),
                  SizedBox(height: ui.h(32 + 16)),
                  ElevatedButton(
                    onPressed: () => _returnToLobby(ref, context),
                    style: GameTheme.playButtonStyle(ui).copyWith(
                      minimumSize: WidgetStateProperty.all(
                        Size(ui.w(180), ui.h(48)),
                      ),
                    ),
                    child: Text('返回大厅', style: TextStyle(fontSize: ui.sp(18))),
                  ),
                ],
              ),
            ),
        ),
      ),
    );
  }
}
