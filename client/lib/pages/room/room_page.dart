import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import '../../controller/room_controller.dart';
import '../../models/room.dart';
import '../../widgets/room_player.dart';

class RoomPage extends ConsumerWidget {
  final String roomId;

  const RoomPage({super.key, required this.roomId});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final room = ref.watch(roomProvider);
    final controller = ref.read(roomControllerProvider);

    return Scaffold(
      appBar: AppBar(
        title: Text('房间 $roomId'),
      ),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          children: [
            Text(
              '等待玩家加入 (${room?.players.length ?? 1}/6)',
              style: Theme.of(context).textTheme.titleLarge,
            ),
            const SizedBox(height: 24),
            Expanded(
              child: GridView.builder(
                gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
                  crossAxisCount: 2,
                  childAspectRatio: 2.5,
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
            const SizedBox(height: 16),
            Row(
              children: [
                Expanded(
                  child: ElevatedButton(
                    onPressed: () => controller.ready(roomId),
                    child: const Text('准备'),
                  ),
                ),
                const SizedBox(width: 12),
                if (room?.isOwner ?? true)
                  Expanded(
                    child: ElevatedButton(
                      onPressed: (room?.allReady ?? false)
                          ? () {
                              controller.startGame(roomId);
                              context.go('/game/$roomId');
                            }
                          : null,
                      style: ElevatedButton.styleFrom(
                        backgroundColor: Colors.amber,
                        foregroundColor: Colors.black,
                      ),
                      child: const Text('开始游戏'),
                    ),
                  ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}
