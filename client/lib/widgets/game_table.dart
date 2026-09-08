import 'package:flutter/material.dart';
import '../models/game_state.dart';
import '../models/player.dart';
import '../utils/seat_layout.dart';
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
    return Stack(
      fit: StackFit.expand,
      children: [
        Center(
          child: FractionallySizedBox(
            widthFactor: SeatLayout.tableWidthFactor,
            heightFactor: SeatLayout.tableHeightFactor,
            child: Container(
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
        ),
        ..._positionPlayers(gameState.players, gameState.mySeatIndex),
      ],
    );
  }

  List<Widget> _positionPlayers(List<Player> players, int mySeatIndex) {
    return players.map((player) {
      final localSeat = SeatLayout.toLocalSeat(player.seatIndex, mySeatIndex);
      final alignment = SeatLayout.alignmentForLocalSeat(localSeat);

      return Align(
        alignment: alignment,
        child: Padding(
          padding: SeatLayout.seatPadding,
          child: PlayerWidget(
            player: player,
            isCurrentTurn: gameState.currentPlayerIndex == player.seatIndex,
          ),
        ),
      );
    }).toList();
  }
}
