import 'package:flutter/material.dart';
import '../models/game_state.dart';
import '../models/player.dart';
import '../theme/game_theme.dart';
import '../utils/seat_layout.dart';
import 'game/empty_seat.dart';
import 'game/played_cards_area.dart';
import 'game/turn_hint.dart';
import 'player_widget.dart';

class GameTableWidget extends StatelessWidget {
  final ClientGameState gameState;
  final String roomId;
  final List<Player> lobbyPlayers;
  final int lobbyMySeatIndex;
  final int maxPlayers;
  final bool isSoloMode;
  final bool isWaitingLobby;
  final void Function(int serverSeatIndex)? onEmptySeatTap;

  const GameTableWidget({
    super.key,
    required this.gameState,
    required this.roomId,
    this.lobbyPlayers = const [],
    this.lobbyMySeatIndex = 0,
    this.maxPlayers = 6,
    this.isSoloMode = false,
    this.isWaitingLobby = false,
    this.onEmptySeatTap,
  });

  @override
  Widget build(BuildContext context) {
    return Stack(
      fit: StackFit.expand,
      children: [
        Container(
          decoration: const BoxDecoration(
            gradient: GameTheme.tableGradient,
          ),
        ),
        Center(
          child: FractionallySizedBox(
            widthFactor: SeatLayout.tableWidthFactor + 0.15,
            heightFactor: SeatLayout.tableHeightFactor + 0.1,
            child: Container(
              decoration: BoxDecoration(
                color: GameTheme.tableBlueMid.withValues(alpha: 0.45),
                borderRadius: BorderRadius.circular(999),
                border: Border.all(
                  color: Colors.white.withValues(alpha: 0.18),
                  width: 2,
                ),
                boxShadow: [
                  BoxShadow(
                    color: Colors.black.withValues(alpha: 0.25),
                    blurRadius: 24,
                    spreadRadius: 2,
                  ),
                ],
              ),
            ),
          ),
        ),
        Center(
          child: isWaitingLobby ? _buildWaitingCenter() : _buildPlayingCenter(),
        ),
        if (isWaitingLobby)
          ..._positionLobbySeats()
        else
          ..._positionPlayers(gameState.players, gameState.mySeatIndex),
      ],
    );
  }

  Widget _buildWaitingCenter() {
    final readyCount = lobbyPlayers.where((p) => p.isReady).length;
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 16),
      decoration: GameTheme.panelDecoration(),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          const Text(
            '等待玩家准备',
            style: TextStyle(
              color: GameTheme.accentGold,
              fontSize: 16,
              fontWeight: FontWeight.bold,
            ),
          ),
          const SizedBox(height: 8),
          Text(
            '${lobbyPlayers.length}/$maxPlayers 人  ·  $readyCount 人已准备',
            style: const TextStyle(color: GameTheme.textSecondary, fontSize: 13),
          ),
          const SizedBox(height: 6),
          Text(
            isSoloMode ? '机器人已就位，点击准备即可开始' : '点击空位可换座',
            style: const TextStyle(color: Colors.white38, fontSize: 11),
          ),
        ],
      ),
    );
  }

  Widget _buildPlayingCenter() {
    final currentPlayer = _currentPlayer();
    final lastPlayerLabel = _lastPlayedPlayerLabel();

    return Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        PlayedCardsArea(
          cardIds: gameState.lastPlayedCards,
          playerLabel: lastPlayerLabel,
        ),
        const SizedBox(height: 16),
        if (currentPlayer != null)
          TurnHintWidget(
            key: ValueKey(gameState.turnId),
            playerName: currentPlayer.nickname,
            isMyTurn: gameState.isMyTurn,
            seconds: 30,
          ),
      ],
    );
  }

  Player? _currentPlayer() {
    if (gameState.players.isEmpty) return null;
    for (final p in gameState.players) {
      if (p.seatIndex == gameState.currentPlayerIndex) return p;
    }
    return gameState.players.first;
  }

  String? _lastPlayedPlayerLabel() {
    if (gameState.lastPlayedCards.isEmpty) return null;
    for (final p in gameState.players) {
      if (p.id == gameState.lastPlayedPlayerIndex) {
        return '${p.nickname} 出牌';
      }
    }
    return '上家出牌';
  }

  List<Widget> _positionLobbySeats() {
    final widgets = <Widget>[];
    for (var serverSeat = 0; serverSeat < maxPlayers; serverSeat++) {
      final localSeat =
          SeatLayout.toLocalSeat(serverSeat, lobbyMySeatIndex, maxPlayers);
      final alignment =
          SeatLayout.alignmentForLocalSeat(localSeat, maxPlayers);
      final player = _playerAtSeat(serverSeat);
      final isSelf = localSeat == 0;

      widgets.add(
        Align(
          alignment: alignment,
          child: Padding(
            padding: SeatLayout.seatPadding,
            child: player != null
                ? PlayerWidget(
                    player: player,
                    compact: isSelf,
                    showLobbyState: true,
                  )
                : EmptySeatWidget(
                    seatIndex: serverSeat,
                    compact: isSelf,
                    onTap: onEmptySeatTap != null
                        ? () => onEmptySeatTap!(serverSeat)
                        : null,
                  ),
          ),
        ),
      );
    }
    return widgets;
  }

  Player? _playerAtSeat(int serverSeat) {
    for (final p in lobbyPlayers) {
      if (p.seatIndex == serverSeat) return p;
    }
    return null;
  }

  List<Widget> _positionPlayers(List<Player> players, int mySeatIndex) {
    return players.map((player) {
      final localSeat =
          SeatLayout.toLocalSeat(player.seatIndex, mySeatIndex, maxPlayers);
      final alignment =
          SeatLayout.alignmentForLocalSeat(localSeat, maxPlayers);
      final isSelf = localSeat == 0;

      return Align(
        alignment: alignment,
        child: Padding(
          padding: SeatLayout.seatPadding,
          child: PlayerWidget(
            player: player,
            isCurrentTurn: gameState.currentPlayerIndex == player.seatIndex,
            compact: isSelf,
          ),
        ),
      );
    }).toList();
  }
}
