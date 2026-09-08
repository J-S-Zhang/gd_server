import 'package:flutter/material.dart';
import '../models/player.dart';
import '../theme/game_theme.dart';

class RoomPlayerWidget extends StatelessWidget {
  final Player? player;
  final int seatIndex;

  const RoomPlayerWidget({super.key, required this.player})
      : seatIndex = 0;

  const RoomPlayerWidget.empty({super.key, required this.seatIndex})
      : player = null;

  @override
  Widget build(BuildContext context) {
    if (player == null) {
      return Container(
        decoration: BoxDecoration(
          border: Border.all(color: Colors.white.withValues(alpha: 0.2)),
          borderRadius: BorderRadius.circular(8),
          color: Colors.white.withValues(alpha: 0.05),
        ),
        child: Center(
          child: Text(
            '座位 ${seatIndex + 1}',
            style: const TextStyle(color: GameTheme.textSecondary),
          ),
        ),
      );
    }

    return Container(
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: player!.isReady
            ? Colors.green.withValues(alpha: 0.25)
            : Colors.white.withValues(alpha: 0.08),
        borderRadius: BorderRadius.circular(8),
        border: Border.all(
          color: player!.isReady ? Colors.greenAccent : Colors.white24,
        ),
      ),
      child: Row(
        children: [
          CircleAvatar(
            backgroundColor: GameTheme.tableBlueLight,
            child: Text(
              player!.nickname.isNotEmpty ? player!.nickname[0] : '?',
              style: const TextStyle(color: Colors.white),
            ),
          ),
          const SizedBox(width: 12),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Text(
                  player!.nickname,
                  style: const TextStyle(
                    fontWeight: FontWeight.bold,
                    color: Colors.white,
                  ),
                ),
                Text(
                  player!.isReady ? '已准备' : '未准备',
                  style: TextStyle(
                    color: player!.isReady ? Colors.greenAccent : GameTheme.textSecondary,
                    fontSize: 12,
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}
