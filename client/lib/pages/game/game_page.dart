import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import '../../config/ui_scale.dart';
import '../../controller/auth_controller.dart';
import '../../controller/game_controller.dart';
import '../../controller/game_notice_controller.dart';
import '../../controller/room_controller.dart';
import '../../controller/seat_chat_controller.dart';
import '../../controller/voice_chat_controller.dart';
import '../../models/game_state.dart';
import '../../models/player.dart';
import '../../models/room.dart' as room_model;
import '../../models/user.dart';
import '../../theme/game_theme.dart';
import '../../utils/hand_layout.dart';
import '../../widgets/game/action_buttons.dart';
import '../../widgets/game/dismiss_vote_dialog.dart';
import '../../widgets/game/game_notice_banner.dart';
import '../../widgets/game/game_top_bar.dart';
import '../../widgets/game/chat_popup_panel.dart';
import '../../widgets/game/dynamic_hand_layout_positioned.dart';
import '../../widgets/game/game_layout_positioned.dart';
import '../../widgets/game/game_settings_sheet.dart';
import '../../widgets/game/hand_toolbar.dart';
import '../../widgets/game/social_toolbar.dart';
import '../../models/seat_round_play.dart';
import '../../utils/seat_layout.dart';
import '../../widgets/game/seat_chat_display.dart';
import '../../widgets/game/seat_played_cards.dart';
import '../../widgets/game/waiting_action_bar.dart';
import '../../widgets/game_table.dart';
import '../../widgets/game/spectate_teammate_bar.dart';
import '../../widgets/hand_cards.dart';
import '../../widgets/player_widget.dart';

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
    ref.read(voiceChatProvider.notifier).listen(roomId: widget.roomId);
    Future.microtask(() {
      ref.read(wsClientProvider).reconnect(widget.roomId);
    });
  }

  @override
  void dispose() {
    ref.read(handCardsMaxHeightProvider.notifier).state = 0;
    ref.read(gameNoticeProvider.notifier).clear();
    ref.read(voiceChatProvider.notifier).reset();
    super.dispose();
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
    final ui = context.ui;
    final layoutPlayers = room?.layoutPlayerCount ?? 4;

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
                      maxPlayers: layoutPlayers,
                      isSoloMode: room?.isSoloMode ?? false,
                      seatChats: seatChats,
                      onEmptySeatTap: isWaitingLobby
                          ? (seatIndex) =>
                              roomController.changeSeat(widget.roomId, seatIndex)
                          : null,
                    ),
                    GameLayoutPositioned(
                      elementId: 'top_bar',
                      child: GameTopBar(
                        roomId: widget.roomId,
                        modeLabel: room?.mode.label ?? '六人掼蛋',
                        myTeamLevel: gameState.myTeamLevel(mySeatIndex % 2),
                        opponentTeamLevel:
                            gameState.opponentTeamLevel(mySeatIndex % 2),
                        onSettings: () =>
                            _showSettingsMenu(context, roomController),
                      ),
                    ),
                    const GameLayoutPositioned(
                      elementId: 'game_notice',
                      child: GameNoticeBanner(),
                    ),
                    const GameLayoutPositioned(
                      elementId: 'social_left',
                      child: SocialToolbar(side: SocialSide.left),
                    ),
                    GameLayoutPositioned(
                      elementId: 'social_right',
                      child: GameVoiceToolbar(roomId: widget.roomId),
                    ),
                    if (isPlaying) ..._buildPlayingOverlays(
                      ui: ui,
                      gameState: gameState,
                      controller: controller,
                      user: user,
                      mySeatIndex: mySeatIndex,
                      maxPlayers: layoutPlayers,
                      me: me,
                      selfChat: selfChat,
                      room: room,
                    ),
                    if (isWaitingLobby)
                      GameLayoutPositioned(
                        elementId: 'waiting_action_bar',
                        child: WaitingActionBar(
                          isReady: me?.isReady ?? false,
                          isOwner: room?.isOwner ?? false,
                          canStart: room?.allReady ?? false,
                          onReady: () => roomController.ready(widget.roomId),
                          onUnready: () => roomController.unready(widget.roomId),
                          onStart: () => roomController.startGame(widget.roomId),
                        ),
                      ),
                  ],
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  void _sendSeatChat(String content, {bool isEmoji = false}) {
    final gameState = ref.read(gameStateProvider);
    final user = ref.read(userProvider);
    final room = ref.read(roomProvider);
    final me = _findMe(room, gameState, user?.id);
    final mySeatIndex = me?.seatIndex ?? 0;
    final seatIndex =
        gameState.isSpectating ? gameState.ownSeatIndex : mySeatIndex;

    ref.read(gameControllerProvider).sendSeatChat(
          widget.roomId,
          seatIndex,
          content,
          isEmoji: isEmoji,
        );
  }

  Player _selfSeatPlayer({
    required bool isSpectating,
    required Player? viewPlayer,
    required Player? me,
    required User? user,
    required int mySeatIndex,
  }) {
    if (isSpectating && viewPlayer != null) {
      return Player(
        id: viewPlayer.id,
        nickname: '${viewPlayer.nickname}（观战）',
        seatIndex: viewPlayer.seatIndex,
        team: viewPlayer.team,
        cardCount: viewPlayer.cardCount,
        hasFinished: viewPlayer.hasFinished,
        finishRank: viewPlayer.finishRank,
        isReady: viewPlayer.isReady,
        isBot: viewPlayer.isBot,
        status: viewPlayer.status,
      );
    }
    if (me != null) return me;
    return Player(
      id: user?.id ?? 0,
      nickname: user?.nickname ?? '玩家',
      seatIndex: mySeatIndex,
      team: mySeatIndex % 2,
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
    required SeatChatMessage? selfChat,
    required room_model.Room? room,
  }) {
    final selfFinished = me?.hasFinished ?? false;
    final isSpectating = gameState.isSpectating;
    final viewSeatIndex = isSpectating ? gameState.mySeatIndex : mySeatIndex;
    final mergedPlayers = <int, Player>{};
    for (final p in [...?room?.players, ...gameState.players]) {
      mergedPlayers[p.id] = p;
    }
    final allPlayers = mergedPlayers.values.toList();
    final viewPlayer = _playerAtSeat(allPlayers, viewSeatIndex);
    final viewPlay =
        gameState.seatRoundPlays[viewSeatIndex] ?? const SeatRoundPlay();
    final isViewSeatTurn = gameState.currentPlayerIndex == viewSeatIndex;
    final hasViewPlay = !viewPlay.isEmpty && !isViewSeatTurn;
    final selfFinishLabel = selfFinished && !isSpectating
        ? SeatLayout.finishRankLabel(me!.finishRank, maxPlayers)
        : '';
    // 本人出完牌后手牌为空，避免显示「等待发牌」；切到队友视角后再显示队友手牌。
    final showHandCards =
        gameState.myCards.isNotEmpty && (isSpectating || !selfFinished);
    if (!showHandCards && ref.read(handCardsMaxHeightProvider) != 0) {
      WidgetsBinding.instance.addPostFrameCallback((_) {
        if (mounted) ref.read(handCardsMaxHeightProvider.notifier).state = 0;
      });
    }
    final selfSeatPlayer = _selfSeatPlayer(
      isSpectating: isSpectating,
      viewPlayer: viewPlayer,
      me: me,
      user: user,
      mySeatIndex: mySeatIndex,
    );

    return [
      if (gameState.isMyTurn)
        DynamicHandGameLayoutPositioned(
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
        DynamicHandGameLayoutPositioned(
          elementId: 'self_play',
          child: SeatPlayedCards(
            play: viewPlay,
            currentLevel: gameState.currentLevel,
            finishLabel: selfFinishLabel.isEmpty ? null : selfFinishLabel,
            layoutElementId: 'self_play',
          ),
        ),
      if (showHandCards)
        DynamicHandGameLayoutPositioned(
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
        elementId: 'self_seat',
        child: PlayerWidget(
          player: selfSeatPlayer,
          isCurrentTurn: gameState.isMyTurn,
          cardCountOverride: isSpectating ? null : gameState.myCards.length,
          chatBubble: ui.seatChatLayoutElement(0, maxPlayers) != null
              ? null
              : selfChat?.content,
          chatIsEmoji: selfChat?.isEmoji ?? false,
          maxPlayers: maxPlayers,
          layoutElementId: 'self_seat',
        ),
      ),
      if (selfChat != null &&
          selfChat.content.isNotEmpty &&
          ui.seatChatLayoutElement(0, maxPlayers) != null)
        Builder(
          builder: (context) {
            final rect = ui.layoutRegionRect('chat_0', maxPlayers: maxPlayers);
            if (rect == null) return const SizedBox.shrink();
            return Positioned(
              left: rect.left,
              top: rect.top,
              width: rect.width,
              height: rect.height,
              child: SeatChatDisplay(
                localSeat: 0,
                maxPlayers: maxPlayers,
                content: selfChat.content,
                isEmoji: selfChat.isEmoji,
              ),
            );
          },
        ),
      if (isSpectating)
        GameLayoutPositioned(
          elementId: 'spectate_bar',
          child: SpectateTeammateBar(
            spectatableTeammates: gameState.spectatableTeammates,
            currentViewSeat: gameState.mySeatIndex,
            players: allPlayers,
            onSelect: (seat) =>
                controller.spectateTeammate(widget.roomId, seat),
          ),
        ),
      GameLayoutPositioned(
        elementId: 'hand_toolbar',
        child: HandToolbar(
          straightFlushSuits: isSpectating
              ? const {}
              : detectStraightFlushSuits(
                  gameState.myCards,
                  currentLevel: gameState.currentLevel,
                ),
          onSuitTap: isSpectating ? null : controller.cycleStraightFlushSelection,
          onRestore: isSpectating ? null : controller.restoreHand,
          onSort: isSpectating ? null : () => controller.sortHand(context),
          externalChatPanel: true,
          showChatPanel: _showChatPanel,
          onShowChatPanelChanged: (v) => setState(() => _showChatPanel = v),
          onQuickMessage: (message) => _sendSeatChat(message),
          onEmoji: (emoji) => _sendSeatChat(emoji, isEmoji: true),
        ),
      ),
      if (_showChatPanel)
        GameLayoutPositioned(
          elementId: 'chat_popup',
          child: ChatPopupPanel(
            selectedTab: _chatTab,
            onTabChanged: (tab) => setState(() => _chatTab = tab),
            onQuickMessageSelected: (message) {
              _sendSeatChat(message);
              setState(() => _showChatPanel = false);
            },
            onEmojiSelected: (emoji) {
              _sendSeatChat(emoji, isEmoji: true);
              setState(() => _showChatPanel = false);
            },
          ),
        ),
    ];
  }

  void _showSettingsMenu(BuildContext context, RoomController roomController) {
    GameSettingsSheet.show(context, roomId: widget.roomId);
  }
}
