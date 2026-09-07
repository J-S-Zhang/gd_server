import 'package:flutter/material.dart';
import '../models/player.dart';

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
          border: Border.all(color: Colors.grey.shade600, style: BorderStyle.solid),
          borderRadius: BorderRadius.circular(8),
        ),
        child: Center(
          child: Text(
            '座位 ${seatIndex + 1}',
            style: TextStyle(color: Colors.grey.shade500),
          ),
        ),
      );
    }

    return Container(
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: player!.isReady ? Colors.green.shade900 : Colors.grey.shade800,
        borderRadius: BorderRadius.circular(8),
        border: Border.all(
          color: player!.isReady ? Colors.green : Colors.grey,
        ),
      ),
      child: Row(
        children: [
          CircleAvatar(
            child: Text(player!.nickname.isNotEmpty ? player!.nickname[0] : '?'),
          ),
          const SizedBox(width: 12),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Text(player!.nickname,
                    style: const TextStyle(fontWeight: FontWeight.bold)),
                Text(
                  player!.isReady ? '已准备' : '未准备',
                  style: TextStyle(
                    color: player!.isReady ? Colors.greenAccent : Colors.grey,
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
