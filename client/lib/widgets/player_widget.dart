import 'package:flutter/material.dart';
import '../models/player.dart';
import '../theme/game_theme.dart';

class PlayerWidget extends StatelessWidget {
  final Player player;
  final bool isCurrentTurn;
  final bool compact;
  final bool showLobbyState;

  const PlayerWidget({
    super.key,
    required this.player,
    this.isCurrentTurn = false,
    this.compact = false,
    this.showLobbyState = false,
  });

  @override
  Widget build(BuildContext context) {
    final level = (player.id % 15) + 5;
    final coins = _formatCoins((player.id * 1379) % 99999 + 1000);

    return Container(
      width: compact ? 72 : 88,
      padding: EdgeInsets.symmetric(horizontal: compact ? 4 : 8, vertical: compact ? 4 : 6),
      decoration: BoxDecoration(
        color: isCurrentTurn
            ? GameTheme.accentGold.withValues(alpha: 0.25)
            : Colors.black.withValues(alpha: 0.35),
        borderRadius: BorderRadius.circular(10),
        border: Border.all(
          color: isCurrentTurn ? GameTheme.accentGold : Colors.white.withValues(alpha: 0.15),
          width: isCurrentTurn ? 2 : 1,
        ),
        boxShadow: isCurrentTurn
            ? [
                BoxShadow(
                  color: GameTheme.accentGold.withValues(alpha: 0.35),
                  blurRadius: 8,
                ),
              ]
            : null,
      ),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Stack(
            clipBehavior: Clip.none,
            children: [
              CircleAvatar(
                radius: compact ? 18 : 22,
                backgroundColor:
                    player.team == 0 ? GameTheme.tableBlueLight : const Color(0xFFE53935),
                child: Text(
                  player.nickname.isNotEmpty ? player.nickname[0] : '?',
                  style: TextStyle(
                    color: Colors.white,
                    fontSize: compact ? 14 : 16,
                    fontWeight: FontWeight.bold,
                  ),
                ),
              ),
              if (isCurrentTurn)
                Positioned(
                  right: -4,
                  top: -4,
                  child: Container(
                    width: 10,
                    height: 10,
                    decoration: const BoxDecoration(
                      color: GameTheme.accentGold,
                      shape: BoxShape.circle,
                    ),
                  ),
                ),
            ],
          ),
          SizedBox(height: compact ? 2 : 4),
          Text(
            player.nickname,
            style: const TextStyle(color: GameTheme.textPrimary, fontSize: 11),
            overflow: TextOverflow.ellipsis,
            maxLines: 1,
          ),
          if (player.isBot)
            const Text(
              '机器人',
              style: TextStyle(color: Colors.cyanAccent, fontSize: 9),
            ),
          if (!compact) ...[
            Text(
              'LV$level',
              style: const TextStyle(color: GameTheme.textSecondary, fontSize: 10),
            ),
            Text(
              coins,
              style: const TextStyle(color: GameTheme.accentGold, fontSize: 10),
            ),
          ],
          Text(
            player.hasFinished
                ? '第${player.finishRank}名'
                : showLobbyState
                    ? (player.isReady ? '已准备' : '未准备')
                    : '${player.cardCount}张',
            style: TextStyle(
              color: player.hasFinished
                  ? GameTheme.accentGold
                  : showLobbyState && player.isReady
                      ? Colors.greenAccent
                      : GameTheme.textSecondary,
              fontSize: 10,
              fontWeight: FontWeight.w600,
            ),
          ),
        ],
      ),
    );
  }

  String _formatCoins(int value) {
    if (value >= 10000) {
      return '${(value / 10000).toStringAsFixed(2)}万';
    }
    return value.toString();
  }
}
