import 'package:flutter/material.dart';
import '../models/player.dart';

class PlayerWidget extends StatelessWidget {
  final Player player;
  final bool isCurrentTurn;

  const PlayerWidget({
    super.key,
    required this.player,
    this.isCurrentTurn = false,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
      decoration: BoxDecoration(
        color: isCurrentTurn
            ? Colors.amber.withValues(alpha: 0.3)
            : Colors.black26,
        borderRadius: BorderRadius.circular(8),
        border: isCurrentTurn
            ? Border.all(color: Colors.amber, width: 2)
            : null,
      ),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          CircleAvatar(
            radius: 20,
            backgroundColor: player.team == 0 ? Colors.blue : Colors.red,
            child: Text(
              player.nickname.isNotEmpty ? player.nickname[0] : '?',
              style: const TextStyle(color: Colors.white),
            ),
          ),
          const SizedBox(height: 4),
          Text(
            player.nickname,
            style: const TextStyle(color: Colors.white, fontSize: 12),
            overflow: TextOverflow.ellipsis,
          ),
          Text(
            player.hasFinished
                ? '第${player.finishRank}名'
                : '${player.cardCount}张',
            style: TextStyle(
              color: player.hasFinished ? Colors.amber : Colors.white70,
              fontSize: 11,
            ),
          ),
        ],
      ),
    );
  }
}
