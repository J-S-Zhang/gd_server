import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../config/ui_scale.dart';
import '../../controller/room_controller.dart';
import '../../models/player.dart';
import '../../theme/game_theme.dart';
import '../../widgets/game/game_layout_positioned.dart';
import '../../widgets/room_player.dart';

class RoomPage extends ConsumerWidget {
  final String roomId;

  const RoomPage({super.key, required this.roomId});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final ui = context.ui;
    final room = ref.watch(roomProvider);
    final controller = ref.read(roomControllerProvider);

    return Scaffold(
      body: Container(
        decoration: const BoxDecoration(gradient: GameTheme.pageGradient),
        child: SafeArea(
          child: Stack(
            fit: StackFit.expand,
            clipBehavior: Clip.hardEdge,
            children: [
              PageLayoutPositioned(
                page: PageLayoutKind.room,
                elementId: 'header',
                child: _buildHeader(ui),
              ),
              PageLayoutPositioned(
                page: PageLayoutKind.room,
                elementId: 'player_grid',
                child: _buildPlayerGrid(ui, room?.players ?? []),
              ),
              PageLayoutPositioned(
                page: PageLayoutKind.room,
                elementId: 'side_panel',
                child: _buildSidePanel(ui, room, controller),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildHeader(UiScale ui) {
    final region = ui.layoutRect(PageLayoutKind.room, 'header');
    final titleSize = region != null ? region.height * 0.45 : ui.sp(ui.config.font.xxl);
    final subtitleSize = region != null ? region.height * 0.28 : ui.sp(ui.config.font.md);

    return Container(
      width: double.infinity,
      padding: EdgeInsets.symmetric(horizontal: region?.width != null ? region!.width * 0.02 : ui.w(16)),
      decoration: BoxDecoration(color: Colors.black.withValues(alpha: 0.2)),
      child: Row(
        children: [
          Text(
            '房间 $roomId',
            style: TextStyle(color: Colors.white, fontSize: titleSize, fontWeight: FontWeight.bold),
          ),
          const Spacer(),
          Text(
            '经典掼蛋 · 六人模式',
            style: TextStyle(color: Colors.white.withValues(alpha: 0.7), fontSize: subtitleSize),
          ),
        ],
      ),
    );
  }

  Widget _buildPlayerGrid(UiScale ui, List<Player> players) {
    final region = ui.layoutRect(PageLayoutKind.room, 'player_grid');
    final pad = region != null ? region.width * 0.04 : ui.w(ui.config.spacing.xl);
    final titleSize = region != null ? region.height * 0.07 : ui.sp(ui.config.font.xl);
    final gap = region != null ? region.width * 0.02 : ui.w(ui.config.spacing.lg);

    final cellW = region != null ? (region.width - pad * 2 - gap * 2) / 3 : 120.0;
    final cellH = region != null ? (region.height - pad * 2 - titleSize - gap * 2) / 2 : 60.0;
    final aspect = cellW / cellH;

    return Container(
      padding: EdgeInsets.all(pad),
      decoration: GameTheme.panelDecoration(ui, radius: region?.height != null ? region!.height * 0.03 : null),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            '等待玩家 (${players.length}/6)',
            style: TextStyle(color: GameTheme.accentGold, fontSize: titleSize, fontWeight: FontWeight.bold),
          ),
          SizedBox(height: gap),
          Expanded(
            child: GridView.builder(
              physics: const NeverScrollableScrollPhysics(),
              gridDelegate: SliverGridDelegateWithFixedCrossAxisCount(
                crossAxisCount: 3,
                childAspectRatio: aspect.clamp(0.5, 4.0),
                crossAxisSpacing: gap,
                mainAxisSpacing: gap,
              ),
              itemCount: 6,
              itemBuilder: (context, index) {
                if (index < players.length) {
                  return RoomPlayerWidget(
                    player: players[index],
                    cellHeight: cellH,
                    cellWidth: cellW,
                  );
                }
                return RoomPlayerWidget.empty(
                  seatIndex: index,
                  cellHeight: cellH,
                  cellWidth: cellW,
                );
              },
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildSidePanel(UiScale ui, room, RoomController controller) {
    final region = ui.layoutRect(PageLayoutKind.room, 'side_panel');
    final pad = region != null ? region.width * 0.08 : ui.w(ui.config.spacing.xl);
    final btnH = region != null ? region.height * 0.16 : ui.h(ui.config.button.largeHeight);
    final btnFont = btnH * 0.38;
    final gap = region != null ? region.height * 0.06 : ui.h(ui.config.spacing.xl);
    final hintSize = region != null ? region.height * 0.07 : ui.sp(ui.config.font.md);

    return Container(
      padding: EdgeInsets.all(pad),
      decoration: GameTheme.panelDecoration(ui, radius: region?.height != null ? region!.height * 0.05 : null),
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          SizedBox(
            width: double.infinity,
            height: btnH,
            child: ElevatedButton(
              onPressed: () => controller.ready(roomId),
              style: GameTheme.hintButtonStyle(ui, minSize: Size(double.infinity, btnH)),
              child: Text('准备', style: TextStyle(fontSize: btnFont)),
            ),
          ),
          SizedBox(height: gap),
          if (room?.isOwner ?? true)
            SizedBox(
              width: double.infinity,
              height: btnH,
              child: ElevatedButton(
                onPressed: (room?.allReady ?? false) ? () => controller.startGame(roomId) : null,
                style: GameTheme.playButtonStyle(ui, minSize: Size(double.infinity, btnH)),
                child: Text('开始游戏', style: TextStyle(fontSize: btnFont)),
              ),
            ),
          SizedBox(height: gap * 1.5),
          Text(
            room?.allReady == true ? '全员已准备，房主可开始' : '等待所有玩家准备',
            style: TextStyle(color: GameTheme.textSecondary, fontSize: hintSize),
            textAlign: TextAlign.center,
          ),
        ],
      ),
    );
  }
}
