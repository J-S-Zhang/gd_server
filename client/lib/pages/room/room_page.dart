import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
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
        child: Row(
          children: [
            Expanded(
              flex: 3,
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    '等待玩家 (${room?.players.length ?? 1}/6)',
                    style: Theme.of(context).textTheme.titleLarge,
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
            const SizedBox(width: 24),
            SizedBox(
              width: 200,
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  SizedBox(
                    width: double.infinity,
                    height: 52,
                    child: ElevatedButton(
                      onPressed: () => controller.ready(roomId),
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
                        style: ElevatedButton.styleFrom(
                          backgroundColor: Colors.amber,
                          foregroundColor: Colors.black,
                        ),
                        child: const Text('开始游戏', style: TextStyle(fontSize: 18)),
                      ),
                    ),
                  const SizedBox(height: 24),
                  Text(
                    room?.allReady == true ? '全员已准备，房主可开始' : '等待所有玩家准备',
                    style: TextStyle(color: Colors.grey.shade400, fontSize: 13),
                    textAlign: TextAlign.center,
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }
}
