import 'package:flutter/material.dart';
import '../config/ui_scale.dart';
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
    final ui = context.ui;
    final seatCfg = ui.config.seatLayout;

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
            widthFactor: seatCfg.tableWidthFactor + seatCfg.tableExtraWidthFactor,
            heightFactor: seatCfg.tableHeightFactor + seatCfg.tableExtraHeightFactor,
            child: Container(
              decoration: BoxDecoration(
                color: GameTheme.tableBlueMid.withValues(alpha: 0.45),
                borderRadius: BorderRadius.circular(999),
                border: Border.all(
                  color: Colors.white.withValues(alpha: 0.18),
                  width: ui.r(2),
                ),
                boxShadow: [
                  BoxShadow(
                    color: Colors.black.withValues(alpha: 0.25),
                    blurRadius: ui.r(24),
                    spreadRadius: ui.r(2),
                  ),
                ],
              ),
            ),
          ),
        ),
        Center(
          child: isWaitingLobby ? _buildWaitingCenter(ui) : _buildPlayingCenter(ui),
        ),
        if (isWaitingLobby)
          ..._positionLobbySeats(ui)
        else
          ..._positionPlayers(ui, _effectivePlayers(), gameState.mySeatIndex),
      ],
    );
  }

  Widget _buildWaitingCenter(UiScale ui) {
    final readyCount = lobbyPlayers.where((p) => p.isReady).length;
    return Container(
      padding: ui.edgeInsetsSymmetric(horizontal: ui.config.spacing.xxl, vertical: ui.config.spacing.xl),
      decoration: GameTheme.panelDecoration(ui),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Text(
            '等待玩家准备',
            style: TextStyle(
              color: GameTheme.accentGold,
              fontSize: ui.sp(ui.config.font.lg),
              fontWeight: FontWeight.bold,
            ),
          ),
          SizedBox(height: ui.h(ui.config.spacing.md)),
          Text(
            '${lobbyPlayers.length}/$maxPlayers 人  ·  $readyCount 人已准备',
            style: TextStyle(color: GameTheme.textSecondary, fontSize: ui.sp(ui.config.font.md)),
          ),
          SizedBox(height: ui.h(ui.config.spacing.sm + 2)),
          Text(
            isSoloMode ? '机器人已就位，点击准备即可开始' : '点击空位可换座',
            style: TextStyle(color: Colors.white38, fontSize: ui.sp(ui.config.font.sm2)),
          ),
        ],
      ),
    );
  }

  Widget _buildPlayingCenter(UiScale ui) {
    final currentPlayer = _currentPlayer();
    final lastPlayerLabel = _lastPlayedPlayerLabel();

    return Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        PlayedCardsArea(
          key: ValueKey(
            '${gameState.lastPlayedPlayerId}_${gameState.lastPlayedCards.join(',')}',
          ),
          cardIds: gameState.lastPlayedCards,
          playerLabel: lastPlayerLabel,
          currentLevel: gameState.currentLevel,
        ),
        SizedBox(height: ui.h(ui.config.spacing.xl)),
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

  List<Player> _effectivePlayers() {
    final merged = <int, Player>{};
    for (final p in lobbyPlayers) {
      merged[p.id] = p;
    }
    for (final p in gameState.players) {
      merged[p.id] = p;
    }
    return merged.values.toList();
  }

  Player? _currentPlayer() {
    final players = _effectivePlayers();
    if (players.isEmpty) return null;
    for (final p in players) {
      if (p.seatIndex == gameState.currentPlayerIndex) return p;
    }
    return players.first;
  }

  String? _lastPlayedPlayerLabel() {
    if (gameState.lastPlayedCards.isEmpty) return null;
    final players = _effectivePlayers();

    if (gameState.lastPlayedPlayerId >= 0) {
      for (final p in players) {
        if (p.id == gameState.lastPlayedPlayerId) {
          return '${p.nickname} 出牌';
        }
      }
    }
    if (gameState.lastPlayedSeatIndex >= 0) {
      for (final p in players) {
        if (p.seatIndex == gameState.lastPlayedSeatIndex) {
          return '${p.nickname} 出牌';
        }
      }
    }
    return '上家出牌';
  }

  List<Widget> _positionLobbySeats(UiScale ui) {
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
            padding: SeatLayout.seatPadding(ui),
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

  List<Widget> _positionPlayers(UiScale ui, List<Player> players, int mySeatIndex) {
    return players.map((player) {
      final localSeat =
          SeatLayout.toLocalSeat(player.seatIndex, mySeatIndex, maxPlayers);
      final alignment =
          SeatLayout.alignmentForLocalSeat(localSeat, maxPlayers);
      final isSelf = localSeat == 0;

      return Align(
        alignment: alignment,
        child: Padding(
          padding: SeatLayout.seatPadding(ui),
          child: PlayerWidget(
            player: player,
            isCurrentTurn: gameState.currentPlayerIndex == player.seatIndex,
            compact: isSelf,
            cardCountOverride: isSelf ? gameState.myCards.length : null,
          ),
        ),
      );
    }).toList();
  }
}
