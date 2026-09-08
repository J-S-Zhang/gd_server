import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../controller/room_controller.dart';
import '../../theme/game_theme.dart';
import '../../widgets/room_player.dart';

class RoomPage extends ConsumerWidget {
  final String roomId;

  const RoomPage({super.key, required this.roomId});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final room = ref.watch(roomProvider);
    final controller = ref.read(roomControllerProvider);

    return Scaffold(
      body: Container(
        decoration: const BoxDecoration(gradient: GameTheme.pageGradient),
        child: SafeArea(
          child: Column(
            children: [
              Container(
                padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
                decoration: BoxDecoration(color: Colors.black.withValues(alpha: 0.2)),
                child: Row(
                  children: [
                    Text(
                      '房间 $roomId',
                      style: const TextStyle(
                        color: Colors.white,
                        fontSize: 20,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    const Spacer(),
                    Text(
                      '经典掼蛋 · 六人模式',
                      style: TextStyle(color: Colors.white.withValues(alpha: 0.7), fontSize: 13),
                    ),
                  ],
                ),
              ),
              Expanded(
                child: Padding(
                  padding: const EdgeInsets.all(16),
                  child: Row(
                    children: [
                      Expanded(
                        flex: 3,
                        child: Container(
                          padding: const EdgeInsets.all(16),
                          decoration: GameTheme.panelDecoration(),
                          child: Column(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: [
                              Text(
                                '等待玩家 (${room?.players.length ?? 1}/6)',
                                style: const TextStyle(
                                  color: GameTheme.accentGold,
                                  fontSize: 18,
                                  fontWeight: FontWeight.bold,
                                ),
                              ),
                              const SizedBox(height: 16),
                              Expanded(
                                child: GridView.builder(
                                  gridDelegate:
                                      const SliverGridDelegateWithFixedCrossAxisCount(
                                    crossAxisCount: 3,
                                    childAspectRatio: 2.2,
                                    crossAxisSpacing: 12,
                                    mainAxisSpacing: 12,
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
                      const SizedBox(width: 20),
                      SizedBox(
                        width: 200,
                        child: Container(
                          padding: const EdgeInsets.all(16),
                          decoration: GameTheme.panelDecoration(),
                          child: Column(
                            mainAxisAlignment: MainAxisAlignment.center,
                            children: [
                              SizedBox(
                                width: double.infinity,
                                height: 52,
                                child: ElevatedButton(
                                  onPressed: () => controller.ready(roomId),
                                  style: GameTheme.hintButtonStyle.copyWith(
                                    minimumSize: WidgetStateProperty.all(
                                      const Size(double.infinity, 52),
                                    ),
                                  ),
                                  child: const Text('准备', style: TextStyle(fontSize: 18)),
                                ),
                              ),
                              const SizedBox(height: 16),
                              if (room?.isOwner ?? true)
                                SizedBox(
                                  width: double.infinity,
                                  height: 52,
                                  child: ElevatedButton(
                                    onPressed: (room?.allReady ?? false)
                                        ? () => controller.startGame(roomId)
                                        : null,
                                    style: GameTheme.playButtonStyle.copyWith(
                                      minimumSize: WidgetStateProperty.all(
                                        const Size(double.infinity, 52),
                                      ),
                                    ),
                                    child: const Text('开始游戏', style: TextStyle(fontSize: 18)),
                                  ),
                                ),
                              const SizedBox(height: 24),
                              Text(
                                room?.allReady == true ? '全员已准备，房主可开始' : '等待所有玩家准备',
                                style: const TextStyle(color: GameTheme.textSecondary, fontSize: 13),
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
