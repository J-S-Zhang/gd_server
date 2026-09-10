import 'package:flutter/material.dart';
import '../config/ui_scale.dart';
import '../models/game_state.dart';
import '../models/player.dart';
import '../models/seat_round_play.dart';
import '../theme/game_theme.dart';
import '../utils/constants.dart';
import '../utils/seat_layout.dart';
import 'countdown.dart';
import 'game/empty_seat.dart';
import 'game/seat_played_cards.dart';
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

    return Stack(
      fit: StackFit.expand,
      children: [
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
    if (_currentPlayer() != null) return const SizedBox.shrink();

    return Container(
      padding: ui.edgeInsetsSymmetric(horizontal: ui.config.spacing.xxl, vertical: ui.config.spacing.xl),
      decoration: GameTheme.panelDecoration(ui),
      child: Text(
        '等待出牌',
        style: TextStyle(color: GameTheme.textSecondary, fontSize: ui.sp(ui.config.font.md2)),
      ),
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
      final seatPlay = gameState.seatRoundPlays[player.seatIndex] ?? const SeatRoundPlay();
      final playedCards = SeatPlayedCards(
        key: ValueKey('seat_play_${player.seatIndex}_${seatPlay.passed}_${seatPlay.cardIds.join(',')}'),
        play: seatPlay,
        currentLevel: gameState.currentLevel,
      );
      final isCurrentTurn = gameState.currentPlayerIndex == player.seatIndex;
      final playBefore = SeatLayout.playBeforePlayer(localSeat, maxPlayers);
      final horizontal = SeatLayout.playHorizontal(localSeat, maxPlayers);
      final hasPlayed = !seatPlay.isEmpty;
      // 本人轮次时倒计时显示在出牌按钮旁，此处不再重复。
      final countdown = isCurrentTurn && !isSelf ? _turnCountdown() : null;

      if (isSelf) {
        if (!isCurrentTurn && !hasPlayed) return const SizedBox.shrink();
        return Align(
          alignment: alignment,
          child: Padding(
            padding: SeatLayout.seatPadding(ui),
            child: _buildSeatLayout(
              ui: ui,
              playBefore: playBefore,
              horizontal: horizontal,
              playedCards: hasPlayed ? playedCards : null,
            ),
          ),
        );
      }

      final playerWidget = PlayerWidget(
        player: player,
        isCurrentTurn: isCurrentTurn,
        compact: isSelf,
        cardCountOverride: isSelf ? gameState.myCards.length : null,
      );

      return Align(
        alignment: alignment,
        child: Padding(
          padding: SeatLayout.seatPadding(ui),
          child: _buildSeatLayout(
            ui: ui,
            playBefore: playBefore,
            horizontal: horizontal,
            playerWidget: playerWidget,
            playedCards: hasPlayed ? playedCards : null,
            countdown: countdown,
          ),
        ),
      );
    }).toList();
  }

  Widget _turnCountdown() {
    return CountdownWidget(
      key: ValueKey('turn_${gameState.turnId}'),
      seconds: Constants.turnTimeoutSeconds,
      circular: true,
    );
  }

  Widget _buildSeatLayout({
    required UiScale ui,
    required bool playBefore,
    required bool horizontal,
    Widget? playerWidget,
    Widget? playedCards,
    Widget? countdown,
  }) {
    final gapW = SizedBox(width: ui.w(ui.config.spacing.sm));
    final gapH = SizedBox(height: ui.h(ui.config.spacing.sm));
    final items = <Widget>[];

    void addIf(Widget? widget) {
      if (widget == null) return;
      if (items.isNotEmpty) items.add(horizontal ? gapW : gapH);
      items.add(widget);
    }

    if (playBefore) {
      addIf(playedCards);
      addIf(countdown);
      addIf(playerWidget);
    } else {
      addIf(playerWidget);
      addIf(countdown);
      addIf(playedCards);
    }

    if (items.isEmpty) return const SizedBox.shrink();

    if (horizontal) {
      return Row(
        mainAxisSize: MainAxisSize.min,
        crossAxisAlignment: CrossAxisAlignment.center,
        children: items,
      );
    }
    return Column(mainAxisSize: MainAxisSize.min, children: items);
  }
}
