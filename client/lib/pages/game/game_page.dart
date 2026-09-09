import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import '../../config/ui_scale.dart';
import '../../controller/auth_controller.dart';
import '../../controller/game_controller.dart';
import '../../controller/room_controller.dart';
import '../../models/game_state.dart';
import '../../models/player.dart';
import '../../models/room.dart' as room_model;
import '../../theme/game_theme.dart';
import '../../widgets/game/action_buttons.dart';
import '../../widgets/game/game_top_bar.dart';
import '../../widgets/game/hand_toolbar.dart';
import '../../widgets/game/social_toolbar.dart';
import '../../widgets/game/waiting_action_bar.dart';
import '../../widgets/game_table.dart';
import '../../widgets/hand_cards.dart';

class GamePage extends ConsumerStatefulWidget {
  final String roomId;

  const GamePage({super.key, required this.roomId});

  @override
  ConsumerState<GamePage> createState() => _GamePageState();
}

class _GamePageState extends ConsumerState<GamePage> {
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

  Player? _findMe(room_model.Room? room, int? userId) {
    if (room == null || userId == null) return null;
    for (final p in room.players) {
      if (p.id == userId) return p;
    }
    return null;
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
    final me = _findMe(room, user?.id);
    final mySeatIndex = me?.seatIndex ?? 0;

    if (gameState.phase == room_model.GamePhase.finished ||
        gameState.phase == room_model.GamePhase.settlement) {
      WidgetsBinding.instance.addPostFrameCallback((_) {
        if (mounted) context.go('/result/${widget.roomId}');
      });
    }

    return Scaffold(
      body: Container(
        decoration: const BoxDecoration(gradient: GameTheme.pageGradient),
        child: SafeArea(
          child: Column(
            children: [
              GameTopBar(
                roomId: widget.roomId,
                modeLabel: room?.mode.label ?? '六人掼蛋',
                gameState: gameState,
                myTeamLevel: gameState.myTeamLevel(mySeatIndex % 2),
                opponentTeamLevel: gameState.opponentTeamLevel(mySeatIndex % 2),
              ),
              Expanded(
                child: Stack(
                  children: [
                    GameTableWidget(
                      gameState: gameState,
                      roomId: widget.roomId,
                      isWaitingLobby: isWaitingLobby,
                      lobbyPlayers: room?.players ?? const [],
                      lobbyMySeatIndex: mySeatIndex,
                      maxPlayers: room?.maxPlayers ?? 6,
                      isSoloMode: room?.isSoloMode ?? false,
                      onEmptySeatTap: isWaitingLobby
                          ? (seatIndex) =>
                              roomController.changeSeat(widget.roomId, seatIndex)
                          : null,
                    ),
                    Positioned(
                      left: ui.w(layout.gameSocialSide),
                      top: ui.h(layout.gameSocialTop),
                      child: const SocialToolbar(side: SocialSide.left),
                    ),
                    Positioned(
                      right: ui.w(layout.gameSocialSide),
                      top: ui.h(layout.gameSocialTop),
                      child: SocialToolbar(
                        side: SocialSide.right,
                        onMore: () => _showMoreMenu(context),
                      ),
                    ),
                    if (isWaitingLobby)
                      Positioned(
                        left: 0,
                        right: 0,
                        bottom: ui.h(layout.gameBottomBar),
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
              if (isPlaying)
                Container(
                  padding: ui.edgeInsetsLTRB(
                    ui.config.spacing.lg,
                    ui.config.spacing.md,
                    ui.config.spacing.lg,
                    ui.config.spacing.md + 2,
                  ),
                  decoration: BoxDecoration(
                    color: Colors.black.withValues(alpha: 0.35),
                    border: Border(
                      top: BorderSide(color: Colors.white.withValues(alpha: 0.08)),
                    ),
                  ),
                  child: Column(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      if (gameState.isMyTurn) ...[
                        Center(
                          child: GameActionButtons(
                            enabled: true,
                            canPlay: controller.selectedCardIds.isNotEmpty,
                            onPass: () => controller.pass(widget.roomId),
                            onHint: () => controller.hint(context),
                            onPlay: () => controller.playCards(
                              widget.roomId,
                              controller.selectedCardIds,
                              context: context,
                            ),
                          ),
                        ),
                        SizedBox(height: ui.h(ui.config.spacing.md)),
                      ],
                      HandCardsWidget(
                        cards: gameState.myCards,
                        currentLevel: gameState.currentLevel,
                        straightStackIds: gameState.handStraightStackIds,
                        onCardTap: controller.toggleCardSelection,
                        onRowHeightChanged: (height) {
                          ref.read(handCardsMaxHeightProvider.notifier).state = height;
                        },
                      ),
                      SizedBox(height: ui.h(ui.config.spacing.sm)),
                      HandToolbar(
                        nickname: user?.nickname ?? '玩家',
                        onSort: () => controller.sortHand(context),
                      ),
                    ],
                  ),
                )
              else
                Container(
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
            ],
          ),
        ),
      ),
    );
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
                context.go('/lobby');
              },
            ),
          ],
        ),
      ),
    );
  }
}
