import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../config/ui_scale.dart';
import '../../controller/room_controller.dart';
import '../../theme/game_theme.dart';
import '../../widgets/room_player.dart';

class RoomPage extends ConsumerWidget {
  final String roomId;

  const RoomPage({super.key, required this.roomId});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final ui = context.ui;
    final layout = ui.config.layout;
    final room = ref.watch(roomProvider);
    final controller = ref.read(roomControllerProvider);

    return Scaffold(
      body: Container(
        decoration: const BoxDecoration(gradient: GameTheme.pageGradient),
        child: SafeArea(
          child: Column(
            children: [
              Container(
                padding: ui.edgeInsetsSymmetric(horizontal: ui.config.spacing.xl, vertical: ui.config.spacing.lg),
                decoration: BoxDecoration(color: Colors.black.withValues(alpha: 0.2)),
                child: Row(
                  children: [
                    Text(
                      '房间 $roomId',
                      style: TextStyle(
                        color: Colors.white,
                        fontSize: ui.sp(ui.config.font.xxl),
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    const Spacer(),
                    Text(
                      '经典掼蛋 · 六人模式',
                      style: TextStyle(
                        color: Colors.white.withValues(alpha: 0.7),
                        fontSize: ui.sp(ui.config.font.md),
                      ),
                    ),
                  ],
                ),
              ),
              Expanded(
                child: Padding(
                  padding: ui.edgeInsetsAll(ui.config.spacing.xl),
                  child: Row(
                    children: [
                      Expanded(
                        flex: 3,
                        child: Container(
                          padding: ui.edgeInsetsAll(ui.config.spacing.xl),
                          decoration: GameTheme.panelDecoration(ui),
                          child: Column(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: [
                              Text(
                                '等待玩家 (${room?.players.length ?? 1}/6)',
                                style: TextStyle(
                                  color: GameTheme.accentGold,
                                  fontSize: ui.sp(ui.config.font.xl),
                                  fontWeight: FontWeight.bold,
                                ),
                              ),
                              SizedBox(height: ui.h(ui.config.spacing.xl)),
                              Expanded(
                                child: GridView.builder(
                                  gridDelegate: SliverGridDelegateWithFixedCrossAxisCount(
                                    crossAxisCount: 3,
                                    childAspectRatio: 2.2,
                                    crossAxisSpacing: ui.w(ui.config.spacing.lg),
                                    mainAxisSpacing: ui.h(ui.config.spacing.lg),
                                  ),
                                  itemCount: 6,
                                  itemBuilder: (context, index) {
                                    final players = room?.players ?? [];
                                    if (index < players.length) {
                                      return RoomPlayerWidget(player: players[index]);
                                    }
                                    return RoomPlayerWidget.empty(seatIndex: index);
                                  },
                                ),
                              ),
                            ],
                          ),
                        ),
                      ),
                      SizedBox(width: ui.w(ui.config.spacing.xl)),
                      SizedBox(
                        width: ui.w(layout.roomSidePanelWidth),
                        child: Container(
                          padding: ui.edgeInsetsAll(ui.config.spacing.xl),
                          decoration: GameTheme.panelDecoration(ui),
                          child: Column(
                            mainAxisAlignment: MainAxisAlignment.center,
                            children: [
                              SizedBox(
                                width: double.infinity,
                                height: ui.h(ui.config.button.largeHeight),
                                child: ElevatedButton(
                                  onPressed: () => controller.ready(roomId),
                                  style: GameTheme.hintButtonStyle(ui).copyWith(
                                    minimumSize: WidgetStateProperty.all(
                                      Size(double.infinity, ui.h(ui.config.button.largeHeight)),
                                    ),
                                  ),
                                  child: Text('准备', style: TextStyle(fontSize: ui.sp(ui.config.font.xl))),
                                ),
                              ),
                              SizedBox(height: ui.h(ui.config.spacing.xl)),
                              if (room?.isOwner ?? true)
                                SizedBox(
                                  width: double.infinity,
                                  height: ui.h(ui.config.button.largeHeight),
                                  child: ElevatedButton(
                                    onPressed: (room?.allReady ?? false)
                                        ? () => controller.startGame(roomId)
                                        : null,
                                    style: GameTheme.playButtonStyle(ui).copyWith(
                                      minimumSize: WidgetStateProperty.all(
                                        Size(double.infinity, ui.h(ui.config.button.largeHeight)),
                                      ),
                                    ),
                                    child: Text('开始游戏', style: TextStyle(fontSize: ui.sp(ui.config.font.xl))),
                                  ),
                                ),
                              SizedBox(height: ui.h(ui.config.spacing.xxl)),
                              Text(
                                room?.allReady == true ? '全员已准备，房主可开始' : '等待所有玩家准备',
                                style: TextStyle(
                                  color: GameTheme.textSecondary,
                                  fontSize: ui.sp(ui.config.font.md),
                                ),
                                textAlign: TextAlign.center,
                              ),
                            ],
                          ),
                        ),
                      ),
                    ],
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
