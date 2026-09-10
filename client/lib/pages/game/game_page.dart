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
import '../../models/user.dart';
import '../../theme/game_theme.dart';
import '../../utils/hand_layout.dart';
import '../../widgets/game/action_buttons.dart';
import '../../widgets/game/dismiss_vote_dialog.dart';
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
  bool _dismissVoteDialogOpen = false;

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
          child: Column(
            children: [
              GameTopBar(
                roomId: widget.roomId,
                modeLabel: room?.mode.label ?? '六人掼蛋',
                gameState: gameState,
                myTeamLevel: gameState.myTeamLevel(mySeatIndex % 2),
                opponentTeamLevel: gameState.opponentTeamLevel(mySeatIndex % 2),
                onSettings: () => _showSettingsMenu(context, roomController),
              ),
              Expanded(
                child: Stack(
                  clipBehavior: Clip.none,
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
                    if (isPlaying)
                      Positioned(
                        left: 0,
                        right: 0,
                        bottom: 0,
                        child: _buildPlayingBottom(
                          ui: ui,
                          gameState: gameState,
                          controller: controller,
                          user: user,
                        ),
                      ),
                  ],
                ),
              ),
              if (!isPlaying)
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

  Widget _buildPlayingBottom({
    required UiScale ui,
    required ClientGameState gameState,
    required GameController controller,
    required User? user,
  }) {
    final layout = ui.config.layout;
    final gap = ui.h(layout.handToolbarHandGap);
    final actionGap = ui.h(layout.gameActionHandGap);

    return Padding(
      padding: ui.edgeInsetsLTRB(
        ui.config.spacing.lg,
        0,
        ui.config.spacing.lg,
        layout.handToolbarBottomPadding,
      ),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          if (gameState.isMyTurn) ...[
            Center(
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
            SizedBox(height: actionGap),
          ],
          HandCardsWidget(
            cards: gameState.myCards,
            currentLevel: gameState.currentLevel,
            organizedGroups: gameState.handOrganizedGroups,
            onCardTap: controller.toggleCardSelection,
            onRowHeightChanged: (height) {
              ref.read(handCardsMaxHeightProvider.notifier).state = height;
            },
          ),
          SizedBox(height: gap),
          HandToolbar(
            nickname: user?.nickname ?? '玩家',
            straightFlushSuits: detectStraightFlushSuits(
              gameState.myCards,
              currentLevel: gameState.currentLevel,
            ),
            onSuitTap: controller.cycleStraightFlushSelection,
            onRestore: controller.restoreHand,
            onSort: () => controller.sortHand(context),
          ),
        ],
      ),
    );
  }

  void _showSettingsMenu(BuildContext context, RoomController roomController) {
    final ui = context.ui;
    final dismissActive = ref.read(dismissVoteProvider) != null;
    showModalBottomSheet(
      context: context,
      backgroundColor: GameTheme.tableBlueDark,
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.vertical(top: Radius.circular(ui.r(ui.config.radius.xl))),
      ),
      builder: (ctx) => Consumer(
        builder: (context, ref, _) {
          final room = ref.watch(roomProvider);
          final canEditTribute =
              room?.isOwner == true && room?.phase == room_model.GamePhase.waiting;
          return SafeArea(
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                SwitchListTile(
                  secondary: Icon(Icons.card_giftcard, color: Colors.white70, size: ui.sp(ui.config.font.xl)),
                  title: Text(
                    '进贡',
                    style: TextStyle(color: Colors.white, fontSize: ui.sp(ui.config.font.md2)),
                  ),
                  subtitle: Text(
                    canEditTribute
                        ? '开启后本局将进行进贡'
                        : room?.isOwner == true
                            ? '对局开始后不可修改'
                            : '仅房主可在开局前设置',
                    style: TextStyle(color: Colors.white54, fontSize: ui.sp(ui.config.font.sm)),
                  ),
                  value: room?.enableTribute ?? false,
                  activeThumbColor: GameTheme.accentGold,
                  onChanged: canEditTribute
                      ? (value) {
                          roomController.setEnableTribute(widget.roomId, value);
                        }
                      : null,
                ),
            ListTile(
              leading: Icon(Icons.group_off, color: Colors.white70, size: ui.sp(ui.config.font.xl)),
              title: Text(
                dismissActive ? '解散投票进行中...' : '申请解散房间',
                style: TextStyle(color: Colors.white, fontSize: ui.sp(ui.config.font.md2)),
              ),
              enabled: !dismissActive,
              onTap: dismissActive
                  ? null
                  : () {
                      Navigator.pop(ctx);
                      roomController.requestDismissRoom(widget.roomId);
                    },
            ),
                ListTile(
                  leading: Icon(Icons.exit_to_app, color: Colors.white70, size: ui.sp(ui.config.font.xl)),
                  title: Text('退出房间', style: TextStyle(color: Colors.white, fontSize: ui.sp(ui.config.font.md2))),
                  onTap: () {
                    Navigator.pop(ctx);
                    roomController.leaveRoom(widget.roomId);
                    ref.read(gameStateProvider.notifier).state = const ClientGameState();
                    context.go('/lobby');
                  },
                ),
              ],
            ),
          );
        },
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
