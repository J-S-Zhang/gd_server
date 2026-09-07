import 'package:flutter/material.dart';
import '../models/game_state.dart';
import '../models/player.dart';
import 'player_widget.dart';

class GameTableWidget extends StatelessWidget {
  final ClientGameState gameState;
  final String roomId;

  const GameTableWidget({
    super.key,
    required this.gameState,
    required this.roomId,
  });

  @override
  Widget build(BuildContext context) {
    final players = gameState.players;

    return Stack(
      children: [
        Center(
          child: Container(
            width: 280,
            height: 180,
            decoration: BoxDecoration(
              color: Colors.green.shade800.withValues(alpha: 0.5),
              borderRadius: BorderRadius.circular(16),
              border: Border.all(color: Colors.amber.withValues(alpha: 0.5)),
            ),
            child: gameState.lastPlayedCards.isNotEmpty
                ? Center(
                    child: Text(
                      '上家出牌: ${gameState.lastPlayedCards.length}张',
                      style: const TextStyle(color: Colors.white70),
                    ),
                  )
                : const Center(
                    child: Text(
                      '等待出牌',
                      style: TextStyle(color: Colors.white38),
                    ),
                  ),
          ),
        ),
        if (players.isNotEmpty)
          ..._positionPlayers(players),
      ],
    );
  }

  List<Widget> _positionPlayers(List<Player> players) {
    final positions = [
      const Alignment(0, 1),    // P0 底部（自己）
      const Alignment(-1, 0.3),   // P1 左下
      const Alignment(-1, -0.3),  // P2 左上
      const Alignment(0, -1),     // P3 顶部
      const Alignment(1, -0.3),     // P4 右上
      const Alignment(1, 0.3),      // P5 右下
    ];

    return List.generate(players.length.clamp(0, 6), (i) {
      return Align(
        alignment: positions[i],
        child: Padding(
          padding: const EdgeInsets.all(8),
          child: PlayerWidget(
            player: players[i],
            isCurrentTurn: gameState.currentPlayerIndex == i,
          ),
        ),
      );
    });
  }
}
