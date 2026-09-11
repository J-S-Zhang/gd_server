import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import '../../config/ui_scale.dart';
import '../../controller/auth_controller.dart';
import '../../controller/game_controller.dart';
import '../../controller/room_controller.dart';
import '../../controller/seat_chat_controller.dart';
import '../../models/game_state.dart';
import '../../models/player.dart';
import '../../models/room.dart' as room_model;
import '../../models/user.dart';
import '../../theme/game_theme.dart';
import '../../utils/hand_layout.dart';
import '../../widgets/game/action_buttons.dart';
import '../../widgets/game/dismiss_vote_dialog.dart';
import '../../widgets/game/game_top_bar.dart';
import '../../widgets/game/chat_popup_panel.dart';
import '../../widgets/game/game_layout_positioned.dart';
import '../../widgets/game/game_settings_sheet.dart';
import '../../widgets/game/hand_toolbar.dart';
import '../../widgets/game/social_toolbar.dart';
import '../../models/seat_round_play.dart';
import '../../utils/seat_layout.dart';
import '../../widgets/game/seat_played_cards.dart';
import '../../widgets/game/waiting_action_bar.dart';
import '../../widgets/game_table.dart';
import '../../widgets/game/spectate_teammate_bar.dart';
import '../../widgets/hand_cards.dart';

class GamePage extends ConsumerStatefulWidget {
  final String roomId;

  const GamePage({super.key, required this.roomId});

  @override
  ConsumerState<GamePage> createState() => _GamePageState();
}

class _GamePageState extends ConsumerState<GamePage> {
  bool _dismissVoteDialogOpen = false;
  bool _showChatPanel = false;
  ChatPanelTab _chatTab = ChatPanelTab.quick;

  @override
  void initState() {
    super.initState();
    ref.read(gameControllerProvider).listen();
    Future.microtask(() {
      ref.read(wsClientProvider).reconnect(widget.roomId);
    });
  }

  bool _isPlaying(room_model.Room? room, ClientGameState gameState) {
    return room?.phase == room_model.GamePhase.playing ||
        gameState.phase == room_model.GamePhase.playing ||
        gameState.phase == room_model.GamePhase.dealing ||
        gameState.myCards.isNotEmpty;
  }

  Player? _findMe(
    room_model.Room? room,
    ClientGameState gameState,
    int? userId,
  ) {
    if (userId == null) return null;

    Player? roomPlayer;
    for (final p in room?.players ?? const <Player>[]) {
      if (p.id == userId) {
        roomPlayer = p;
        break;
      }
    }

    Player? gamePlayer;
    for (final p in gameState.players) {
      if (p.id == userId) {
        gamePlayer = p;
        break;
      }
    }

    if (roomPlayer == null) return gamePlayer;
    if (gamePlayer == null) return roomPlayer;

    return Player(
      id: roomPlayer.id,
      nickname: roomPlayer.nickname,
      seatIndex: roomPlayer.seatIndex,
      team: roomPlayer.team,
      cardCount: gamePlayer.cardCount,
      hasFinished: gamePlayer.hasFinished,
      finishRank: gamePlayer.finishRank,
      isReady: roomPlayer.isReady,
      isBot: roomPlayer.isBot,
      status: roomPlayer.status,
    );
  }

  @override
  Widget build(BuildContext context) {
    final ui = context.ui;
    final layout = ui.config.layout;
    final gameState = ref.watch(gameStateProvider);
    final room = ref.watch(roomProvider);
    final controller = ref.read(gameControllerProvider);
    final roomController = ref.read(roomControllerProvider);
    final user = ref.watch(userProvider);

    final isPlaying = _isPlaying(room, gameState);
    final isWaitingLobby = !isPlaying;
    final me = _findMe(room, gameState, user?.id);
    final mySeatIndex = me?.seatIndex ?? 0;
    final seatChats = ref.watch(seatChatProvider);
    final selfChat = seatChats[mySeatIndex];

    if (gameState.phase == room_model.GamePhase.finished ||
        gameState.phase == room_model.GamePhase.settlement) {
      WidgetsBinding.instance.addPostFrameCallback((_) {
        if (mounted) context.go('/result/${widget.roomId}');
      });
    }

    ref.listen(dismissVoteProvider, (prev, next) {
      if (!mounted) return;
      if (next == null) {
        if (_dismissVoteDialogOpen) {
          Navigator.of(context, rootNavigator: true).maybePop();
          _dismissVoteDialogOpen = false;
        }
        return;
      }
      if (_dismissVoteDialogOpen) return;
      _dismissVoteDialogOpen = true;
      showDialog<void>(
        context: context,
        barrierDismissible: false,
        builder: (ctx) => DismissVoteDialog(roomId: widget.roomId),
      ).whenComplete(() {
        _dismissVoteDialogOpen = false;
      });
    });

    return Scaffold(
      body: GameTheme.gameBackground(
        child: SafeArea(
          child: Stack(
            fit: StackFit.expand,
            clipBehavior: Clip.hardEdge,
            children: [
              Positioned.fill(
                child: Stack(
                  fit: StackFit.expand,
                  clipBehavior: Clip.hardEdge,
                  children: [
                    GameTableWidget(
                      gameState: gameState,
                      roomId: widget.roomId,
                      isWaitingLobby: isWaitingLobby,
                      lobbyPlayers: room?.players ?? const [],
                      lobbyMySeatIndex: mySeatIndex,
                      maxPlayers: room?.maxPlayers ?? 6,
                      isSoloMode: room?.isSoloMode ?? false,
                      seatChats: seatChats,
                      onEmptySeatTap: isWaitingLobby
                          ? (seatIndex) =>
                              roomController.changeSeat(widget.roomId, seatIndex)
                          : null,
                    ),
                    _layoutPositioned(
                      ui: ui,
                      elementId: 'social_left',
                      defaultLeft: layout.gameSocialSide,
                      defaultTop: layout.gameSocialTop,
                      child: const SocialToolbar(side: SocialSide.left),
                    ),
                    _layoutPositioned(
                      ui: ui,
                      elementId: 'social_right',
                      defaultRight: layout.gameSocialSide,
                      defaultTop: layout.gameSocialTop,
                      child: SocialToolbar(
                        side: SocialSide.right,
                        onMore: () => _showMoreMenu(context),
                      ),
                    ),
                    if (isPlaying) ..._buildPlayingOverlays(
                      ui: ui,
                      gameState: gameState,
                      controller: controller,
                      user: user,
                      mySeatIndex: mySeatIndex,
                      maxPlayers: room?.maxPlayers ?? 6,
                      me: me,
                      selfChat: selfChat,
                      room: room,
                    ),
                    if (isWaitingLobby)
                      Positioned(
                        left: 0,
                        right: 0,
                        bottom: ui.h(layout.gameBottomBar) +
                            ui.h(ui.config.spacing.md + 2) * 2 +
                            ui.sp(ui.config.font.md) * 1.4,
                        child: Center(
                          child: WaitingActionBar(
                            isReady: me?.isReady ?? false,
                            isOwner: room?.isOwner ?? false,
                            canStart: room?.allReady ?? false,
                            onReady: () => roomController.ready(widget.roomId),
                            onUnready: () => roomController.unready(widget.roomId),
                            onStart: () => roomController.startGame(widget.roomId),
                          ),
                        ),
                      ),
                  ],
                ),
              ),
              Positioned(
                top: 0,
                left: 0,
                right: 0,
                child: GameTopBar(
                  roomId: widget.roomId,
                  modeLabel: room?.mode.label ?? '六人掼蛋',
                  gameState: gameState,
                  myTeamLevel: gameState.myTeamLevel(mySeatIndex % 2),
                  opponentTeamLevel: gameState.opponentTeamLevel(mySeatIndex % 2),
                  onSettings: () => _showSettingsMenu(context, roomController),
                ),
              ),
              if (!isPlaying)
                Positioned(
                  left: 0,
                  right: 0,
                  bottom: 0,
                  child: Container(
                    width: double.infinity,
                    padding: ui.edgeInsetsSymmetric(vertical: ui.config.spacing.md + 2),
                    color: Colors.black.withValues(alpha: 0.25),
                    child: Text(
                      me == null
                          ? '正在同步房间信息...'
                          : '您当前在座位 ${mySeatIndex + 1}  ·  ${me.isReady ? '已准备' : '未准备'}',
                      textAlign: TextAlign.center,
                      style: TextStyle(
                        color: GameTheme.textSecondary,
                        fontSize: ui.sp(ui.config.font.md),
                      ),
                    ),
                  ),
                ),
            ],
          ),
        ),
      ),
    );
  }

  void _sendSeatChat(int seatIndex, String content, {bool isEmoji = false}) {
    ref.read(seatChatProvider.notifier).show(
          seatIndex,
          content,
          isEmoji: isEmoji,
        );
  }

  Widget _layoutPositioned({
    required UiScale ui,
    required String elementId,
    required Widget child,
    double? defaultLeft,
    double? defaultRight,
    required double defaultTop,
  }) {
    final rect = ui.elementRect(elementId);
    if (rect != null) {
      return Positioned(
        left: rect.left,
        top: rect.top,
        width: rect.width,
        height: rect.height,
        child: child,
      );
    }

    final anchor = ui.config.gamePageLayout.element(elementId);
    if (anchor == null) {
      return Positioned(
        left: defaultLeft != null ? ui.w(defaultLeft) : null,
        right: defaultRight != null ? ui.w(defaultRight) : null,
        top: ui.h(defaultTop),
        child: child,
      );
    }

    return Positioned(
      left: ui.layoutX(anchor.x),
      top: ui.layoutY(anchor.y),
      child: child,
    );
  }

  Player? _playerAtSeat(List<Player> players, int seatIndex) {
    for (final p in players) {
      if (p.seatIndex == seatIndex) return p;
    }
    return null;
  }

  List<Widget> _buildPlayingOverlays({
    required UiScale ui,
    required ClientGameState gameState,
    required GameController controller,
    required User? user,
    required int mySeatIndex,
    required int maxPlayers,
    required Player? me,
    required SeatChatDisplay? selfChat,
    required room_model.Room? room,
  }) {
    final selfFinished = me?.hasFinished ?? false;
    final isSpectating = gameState.isSpectating;
    final viewSeatIndex = isSpectating ? gameState.mySeatIndex : mySeatIndex;
    final chatSeatIndex = isSpectating ? gameState.ownSeatIndex : mySeatIndex;
    final mergedPlayers = <int, Player>{};
    for (final p in [...?room?.players, ...gameState.players]) {
      mergedPlayers[p.id] = p;
    }
    final allPlayers = mergedPlayers.values.toList();
    final viewPlayer = _playerAtSeat(allPlayers, viewSeatIndex);
    final viewPlay =
        gameState.seatRoundPlays[viewSeatIndex] ?? const SeatRoundPlay();
    final hasViewPlay = !viewPlay.isEmpty;
    final selfFinishLabel = selfFinished && !isSpectating
        ? SeatLayout.finishRankLabel(me!.finishRank, maxPlayers)
        : '';
    final showHandCards = !selfFinished || isSpectating;

    return [
      if (gameState.isMyTurn)
        GameLayoutPositioned(
          elementId: 'action_buttons',
          child: GameActionButtons(
            enabled: true,
            canPlay: controller.selectedCardIds.isNotEmpty,
            turnId: gameState.turnId,
            onPass: gameState.mustRespondToTrick(user?.id)
                ? () => controller.pass(widget.roomId)
                : null,
            onHint: () => controller.hint(context),
            onPlay: () => controller.playCards(
              widget.roomId,
              controller.selectedCardIds,
              context: context,
            ),
          ),
        ),
      if (hasViewPlay || (selfFinished && !isSpectating))
        GameLayoutPositioned(
          elementId: 'self_play',
          child: SeatPlayedCards(
            play: viewPlay,
            currentLevel: gameState.currentLevel,
            finishLabel: selfFinishLabel.isEmpty ? null : selfFinishLabel,
            layoutElementId: 'self_play',
          ),
        ),
      if (showHandCards)
        GameLayoutPositioned(
          elementId: 'hand_cards',
          child: HandCardsWidget(
            cards: gameState.myCards,
            currentLevel: gameState.currentLevel,
            organizedGroups: isSpectating ? const [] : gameState.handOrganizedGroups,
            readOnly: isSpectating,
            onCardTap: controller.toggleCardSelection,
            onRowHeightChanged: (height) {
              ref.read(handCardsMaxHeightProvider.notifier).state = height;
            },
          ),
        ),
      GameLayoutPositioned(
        elementId: 'hand_toolbar',
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            if (isSpectating)
              Padding(
                padding: EdgeInsets.only(bottom: ui.h(ui.config.spacing.sm)),
                child: SpectateTeammateBar(
                  spectatableTeammates: gameState.spectatableTeammates,
                  currentViewSeat: gameState.mySeatIndex,
                  players: allPlayers,
                  onSelect: (seat) =>
                      controller.spectateTeammate(widget.roomId, seat),
                ),
              ),
            HandToolbar(
              nickname: isSpectating
                  ? '${viewPlayer?.nickname ?? '队友'}（观战）'
                  : (user?.nickname ?? '玩家'),
              straightFlushSuits: isSpectating
                  ? const {}
                  : detectStraightFlushSuits(
                      gameState.myCards,
                      currentLevel: gameState.currentLevel,
                    ),
              onSuitTap: isSpectating ? null : controller.cycleStraightFlushSelection,
              onRestore: isSpectating ? null : controller.restoreHand,
              onSort: isSpectating ? null : () => controller.sortHand(context),
              chatBubble: selfChat?.content,
              chatIsEmoji: selfChat?.isEmoji ?? false,
              externalChatPanel: true,
              showChatPanel: _showChatPanel,
              onShowChatPanelChanged: (v) => setState(() => _showChatPanel = v),
              onQuickMessage: (message) => _sendSeatChat(chatSeatIndex, message),
              onEmoji: (emoji) =>
                  _sendSeatChat(chatSeatIndex, emoji, isEmoji: true),
            ),
          ],
        ),
      ),
      if (_showChatPanel)
        GameLayoutPositioned(
          elementId: 'chat_popup',
          child: ChatPopupPanel(
            selectedTab: _chatTab,
            onTabChanged: (tab) => setState(() => _chatTab = tab),
            onQuickMessageSelected: (message) {
              _sendSeatChat(mySeatIndex, message);
              setState(() => _showChatPanel = false);
            },
            onEmojiSelected: (emoji) {
              _sendSeatChat(mySeatIndex, emoji, isEmoji: true);
              setState(() => _showChatPanel = false);
            },
          ),
        ),
    ];
  }

  void _showSettingsMenu(BuildContext context, RoomController roomController) {
    GameSettingsSheet.show(context, roomId: widget.roomId);
  }

  void _showMoreMenu(BuildContext context) {
    final ui = context.ui;
    showModalBottomSheet(
      context: context,
      backgroundColor: GameTheme.tableBlueDark,
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.vertical(top: Radius.circular(ui.r(ui.config.radius.xl))),
      ),
      builder: (ctx) => SafeArea(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            ListTile(
              leading: Icon(Icons.help_outline, color: Colors.white70, size: ui.sp(ui.config.font.xl)),
              title: Text('游戏帮助', style: TextStyle(color: Colors.white, fontSize: ui.sp(ui.config.font.md2))),
              onTap: () => Navigator.pop(ctx),
            ),
            ListTile(
              leading: Icon(Icons.settings, color: Colors.white70, size: ui.sp(ui.config.font.xl)),
              title: Text('设置', style: TextStyle(color: Colors.white, fontSize: ui.sp(ui.config.font.md2))),
              onTap: () => Navigator.pop(ctx),
            ),
            ListTile(
              leading: Icon(Icons.exit_to_app, color: Colors.white70, size: ui.sp(ui.config.font.xl)),
              title: Text('退出房间', style: TextStyle(color: Colors.white, fontSize: ui.sp(ui.config.font.md2))),
              onTap: () {
                Navigator.pop(ctx);
                ref.read(roomControllerProvider).leaveRoom(widget.roomId);
                ref.read(gameStateProvider.notifier).state = const ClientGameState();
                context.go('/lobby');
              },
            ),
          ],
        ),
      ),
    );
  }
}
