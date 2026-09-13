import 'package:flutter/material.dart';
import '../config/ui_config.dart';
import '../config/ui_scale.dart';
import '../controller/seat_chat_controller.dart';
import 'game/game_layout_positioned.dart';
import '../models/game_state.dart';
import '../models/player.dart';
import '../models/seat_round_play.dart';
import '../theme/game_theme.dart';
import '../utils/constants.dart';
import 'countdown.dart';
import '../utils/seat_layout.dart';
import 'game/empty_seat.dart';
import 'game/seat_chat_display.dart';
import 'game/seat_play_area.dart';
import 'game/region_child_stack.dart';
import 'game/region_fit_text.dart';
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
  final Map<int, SeatChatMessage> seatChats;

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
    this.seatChats = const <int, SeatChatMessage>{},
  });

  @override
  Widget build(BuildContext context) {
    final ui = context.ui;

    final centerRect = ui.layoutRegionRect('table_center', maxPlayers: maxPlayers);

    return Stack(
      fit: StackFit.expand,
      children: [
        if (centerRect != null)
          Positioned(
            left: centerRect.left,
            top: centerRect.top,
            width: centerRect.width,
            height: centerRect.height,
            child: isWaitingLobby
                ? _buildWaitingCenter(ui)
                : _buildPlayingCenter(ui),
          )
        else
          Center(
            child: isWaitingLobby
                ? _buildWaitingCenter(ui)
                : _buildPlayingCenter(ui),
          ),
        if (isWaitingLobby)
          ..._positionLobbySeats(ui)
        else ...[
          ..._positionPlayers(ui, _effectivePlayers(), gameState.mySeatIndex),
          ..._positionPlayAreas(ui, _effectivePlayers(), gameState.mySeatIndex),
          ..._positionChatAreas(ui, _effectivePlayers(), gameState.mySeatIndex),
        ],
      ],
    );
  }

  static const _centerId = 'table_center';

  Widget _buildWaitingCenter(UiScale ui) {
    final readyCount = lobbyPlayers.where((p) => p.isReady).length;
    final parentEl = ui.config.gamePageLayout.element(_centerId);
    final useChildren = parentEl != null && parentEl.children.containsKey('title');

    if (useChildren) {
      final titleSize = ui.regionFontSize(_centerId, childId: 'title', heightRatio: 0.55);
      final subSize = ui.regionFontSize(_centerId, childId: 'subtitle', heightRatio: 0.5);
      final hintSize = ui.regionFontSize(_centerId, childId: 'hint', heightRatio: 0.45);

      return DecoratedBox(
        decoration: GameTheme.panelDecoration(ui),
        child: Stack(
          children: [
            RegionChildPositioned(
              parentId: _centerId,
              childId: 'title',
              child: RegionFitText(
                text: '等待玩家准备',
                fontSize: titleSize,
                layoutParentId: _centerId,
                layoutChildId: 'title',
                color: GameTheme.accentGold,
                fontWeight: FontWeight.bold,
              ),
            ),
            RegionChildPositioned(
              parentId: _centerId,
              childId: 'subtitle',
              child: RegionFitText(
                text: '${lobbyPlayers.length}/$maxPlayers 人  ·  $readyCount 人已准备',
                fontSize: subSize,
                layoutParentId: _centerId,
                layoutChildId: 'subtitle',
                color: GameTheme.textSecondary,
              ),
            ),
            RegionChildPositioned(
              parentId: _centerId,
              childId: 'hint',
              child: RegionFitText(
                text: isSoloMode ? '机器人已就位，点击准备即可开始' : '点击空位可换座',
                fontSize: hintSize,
                layoutParentId: _centerId,
                layoutChildId: 'hint',
                color: Colors.white38,
              ),
            ),
          ],
        ),
      );
    }

    final titleSize = ui.regionFontSize(_centerId, heightRatio: 0.42);
    final subSize = ui.regionFontSize(_centerId, heightRatio: 0.28);
    return Container(
      padding: ui.edgeInsetsSymmetric(
        horizontal: ui.config.spacing.xxl,
        vertical: ui.config.spacing.xl,
      ),
      decoration: GameTheme.panelDecoration(ui),
      child: RegionFitTextBlock(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Text(
              '等待玩家准备',
              style: TextStyle(
                color: GameTheme.accentGold,
                fontSize: titleSize,
                fontWeight: FontWeight.bold,
              ),
            ),
            SizedBox(height: ui.h(ui.config.spacing.md)),
            Text(
              '${lobbyPlayers.length}/$maxPlayers 人  ·  $readyCount 人已准备',
              style: TextStyle(color: GameTheme.textSecondary, fontSize: subSize),
            ),
            SizedBox(height: ui.h(ui.config.spacing.sm + 2)),
            Text(
              isSoloMode ? '机器人已就位，点击准备即可开始' : '点击空位可换座',
              style: TextStyle(color: Colors.white38, fontSize: subSize * 0.85),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildPlayingCenter(UiScale ui) {
    if (_currentPlayer() != null) return const SizedBox.shrink();

    final fontSize = ui.regionFontSize(_centerId, childId: 'title', heightRatio: 0.5);
    return DecoratedBox(
      decoration: GameTheme.panelDecoration(ui),
      child: RegionFitText(
        text: '等待出牌',
        fontSize: fontSize,
        layoutParentId: _centerId,
        layoutChildId: 'title',
        color: GameTheme.textSecondary,
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

  /// 将座位组件定位在 20:9 画布坐标系内（与出牌区、顶栏等一致）。
  Widget _positionSeatOnCanvas({
    required UiScale ui,
    required int localSeat,
    required Widget child,
    List<GamePageElementLayout>? seatElements,
  }) {
    final seatLayoutId = 'seat_$localSeat';
    final rect = ui.layoutRegionRect(seatLayoutId, maxPlayers: maxPlayers);
    if (rect != null) {
      return Positioned(
        left: rect.left,
        top: rect.top,
        width: rect.width,
        height: rect.height,
        child: child,
      );
    }

    final alignment = SeatLayout.alignmentForLocalSeat(
      localSeat,
      maxPlayers,
      seatElements: seatElements,
    );
    return Align(
      alignment: alignment,
      child: Padding(
        padding: SeatLayout.seatPadding(ui),
        child: child,
      ),
    );
  }

  List<Widget> _positionLobbySeats(UiScale ui) {
    final seatElements = ui.config.seatLayout.seatElementsFor(maxPlayers);
    final widgets = <Widget>[];
    for (var serverSeat = 0; serverSeat < maxPlayers; serverSeat++) {
      final localSeat =
          SeatLayout.toLocalSeat(serverSeat, lobbyMySeatIndex, maxPlayers);
      if (localSeat == 0) continue;

      final player = _playerAtSeat(serverSeat);

      widgets.add(
        _positionSeatOnCanvas(
          ui: ui,
          localSeat: localSeat,
          seatElements: seatElements,
          child: player != null
              ? PlayerWidget(
                  player: player,
                  showLobbyState: true,
                  layoutElementId: 'seat_$localSeat',
                  maxPlayers: maxPlayers,
                )
              : EmptySeatWidget(
                  seatIndex: serverSeat,
                  layoutElementId: 'seat_$localSeat',
                  maxPlayers: maxPlayers,
                  onTap: onEmptySeatTap != null
                      ? () => onEmptySeatTap!(serverSeat)
                      : null,
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
    final seatElements = ui.config.seatLayout.seatElementsFor(maxPlayers);
    return players.map((player) {
      final localSeat =
          SeatLayout.toLocalSeat(player.seatIndex, mySeatIndex, maxPlayers);
      final isSelf = localSeat == 0;
      final seatPlay = gameState.seatRoundPlays[player.seatIndex] ?? const SeatRoundPlay();
      final isCurrentTurn = gameState.currentPlayerIndex == player.seatIndex;
      final playBefore = SeatLayout.playBeforePlayer(localSeat, maxPlayers);
      final horizontal = SeatLayout.playHorizontal(localSeat, maxPlayers);
      final hasPlayed = !seatPlay.isEmpty;
      final seatChat = seatChats[player.seatIndex];
      final seatLayoutId = 'seat_$localSeat';
      final usesConfiguredPlayArea = _hasConfiguredPlayArea(ui, localSeat);

      if (isSelf) {
        return const SizedBox.shrink();
      }

      final playerWidget = PlayerWidget(
        player: player,
        isCurrentTurn: isCurrentTurn,
        cardCountOverride: isSelf ? gameState.myCards.length : null,
        nicknamePlacement: SeatLayout.nicknamePlacement(localSeat, maxPlayers),
        finishRankPlacement: SeatLayout.finishRankPlacement(localSeat, maxPlayers),
        chatBubble: _usesConfiguredChatArea(ui, localSeat) ? null : seatChat?.content,
        chatIsEmoji: seatChat?.isEmoji ?? false,
        maxPlayers: maxPlayers,
        layoutElementId: seatLayoutId,
      );

      if (usesConfiguredPlayArea) {
        return _positionSeatOnCanvas(
          ui: ui,
          localSeat: localSeat,
          seatElements: seatElements,
          child: playerWidget,
        );
      }

      final playedCards = SeatPlayedCards(
        key: ValueKey(
          'seat_play_${player.seatIndex}_${seatPlay.passed}_${seatPlay.cardIds.join(',')}',
        ),
        play: seatPlay,
        currentLevel: gameState.currentLevel,
        layoutElementId: seatLayoutId,
        maxPlayers: maxPlayers,
      );

      return _positionSeatOnCanvas(
        ui: ui,
        localSeat: localSeat,
        seatElements: seatElements,
        child: Center(
          child: _buildSeatLayout(
            ui: ui,
            playBefore: playBefore,
            horizontal: horizontal,
            playerWidget: playerWidget,
            playedCards: hasPlayed ? playedCards : null,
            countdown: isCurrentTurn ? _legacyTurnCountdown() : null,
          ),
        ),
      );
    }).toList();
  }

  List<Widget> _positionPlayAreas(
    UiScale ui,
    List<Player> players,
    int mySeatIndex,
  ) {
    return players.map((player) {
      final localSeat =
          SeatLayout.toLocalSeat(player.seatIndex, mySeatIndex, maxPlayers);
      if (localSeat == 0 || !_hasConfiguredPlayArea(ui, localSeat)) {
        return const SizedBox.shrink();
      }

      final seatPlay =
          gameState.seatRoundPlays[player.seatIndex] ?? const SeatRoundPlay();
      final isCurrentTurn = gameState.currentPlayerIndex == player.seatIndex;
      if (seatPlay.isEmpty && !isCurrentTurn) {
        return const SizedBox.shrink();
      }

      final playLayoutId = 'play_$localSeat';
      final rect = ui.layoutRegionRect(playLayoutId, maxPlayers: maxPlayers);
      if (rect == null) return const SizedBox.shrink();

      return Positioned(
        left: rect.left,
        top: rect.top,
        width: rect.width,
        height: rect.height,
        child: SeatPlayArea(
          localSeat: localSeat,
          maxPlayers: maxPlayers,
          play: seatPlay,
          currentLevel: gameState.currentLevel,
          showCountdown: isCurrentTurn,
          turnId: gameState.turnId,
        ),
      );
    }).toList();
  }

  bool _hasConfiguredPlayArea(UiScale ui, int localSeat) {
    return ui.seatPlayLayoutElement(localSeat, maxPlayers) != null;
  }

  List<Widget> _positionChatAreas(
    UiScale ui,
    List<Player> players,
    int mySeatIndex,
  ) {
    return players.map((player) {
      final localSeat =
          SeatLayout.toLocalSeat(player.seatIndex, mySeatIndex, maxPlayers);
      if (localSeat == 0 || !_usesConfiguredChatArea(ui, localSeat)) {
        return const SizedBox.shrink();
      }

      final seatChat = seatChats[player.seatIndex];
      if (seatChat == null || seatChat.content.isEmpty) {
        return const SizedBox.shrink();
      }

      final chatLayoutId = 'chat_$localSeat';
      final rect = ui.layoutRegionRect(chatLayoutId, maxPlayers: maxPlayers);
      if (rect == null) return const SizedBox.shrink();

      return Positioned(
        left: rect.left,
        top: rect.top,
        width: rect.width,
        height: rect.height,
        child: SeatChatDisplay(
          localSeat: localSeat,
          maxPlayers: maxPlayers,
          content: seatChat.content,
          isEmoji: seatChat.isEmoji,
        ),
      );
    }).toList();
  }

  bool _usesConfiguredChatArea(UiScale ui, int localSeat) {
    return ui.seatChatLayoutElement(localSeat, maxPlayers) != null;
  }

  Widget _legacyTurnCountdown() {
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
