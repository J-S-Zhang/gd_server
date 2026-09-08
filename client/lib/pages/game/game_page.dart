import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
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
                    const Positioned(
                      left: 8,
                      top: 80,
                      child: SocialToolbar(side: SocialSide.left),
                    ),
                    Positioned(
                      right: 8,
                      top: 80,
                      child: SocialToolbar(
                        side: SocialSide.right,
                        onMore: () => _showMoreMenu(context),
                      ),
                    ),
                    Positioned(
                      left: 0,
                      right: 0,
                      bottom: 8,
                      child: Center(
                        child: isWaitingLobby
                            ? WaitingActionBar(
                                isReady: me?.isReady ?? false,
                                isOwner: room?.isOwner ?? false,
                                canStart: room?.allReady ?? false,
                                onReady: () => roomController.ready(widget.roomId),
                                onUnready: () => roomController.unready(widget.roomId),
                                onStart: () => roomController.startGame(widget.roomId),
                              )
                            : GameActionButtons(
                                enabled: gameState.isMyTurn,
                                canPlay: controller.selectedCardIds.isNotEmpty,
                                onPass: () => controller.pass(widget.roomId),
                                onHint: () => controller.hint(context),
                                onPlay: () => controller.playCards(
                                  widget.roomId,
                                  controller.selectedCardIds,
                                ),
                              ),
                      ),
                    ),
                  ],
                ),
              ),
              if (isPlaying)
                Container(
                  padding: const EdgeInsets.fromLTRB(12, 8, 12, 10),
                  decoration: BoxDecoration(
                    color: Colors.black.withValues(alpha: 0.35),
                    border: Border(
                      top: BorderSide(color: Colors.white.withValues(alpha: 0.08)),
                    ),
                  ),
                  child: Column(
                    children: [
                      HandToolbar(
                        nickname: user?.nickname ?? '玩家',
                        coinLabel: '6.14万',
                        onSort: controller.sortHand,
                        onAutoSort: controller.autoSortHand,
                        onMore: () => _showMoreMenu(context),
                      ),
                      const SizedBox(height: 8),
                      HandCardsWidget(
                        cards: gameState.myCards,
                        currentLevel: gameState.currentLevel,
                        onCardTap: controller.toggleCardSelection,
                      ),
                    ],
                  ),
                )
              else
                Container(
                  width: double.infinity,
                  padding: const EdgeInsets.symmetric(vertical: 10),
                  color: Colors.black.withValues(alpha: 0.25),
                  child: Text(
                    me == null
                        ? '正在同步房间信息...'
                        : '您当前在座位 ${mySeatIndex + 1}  ·  ${me.isReady ? '已准备' : '未准备'}',
                    textAlign: TextAlign.center,
                    style: const TextStyle(color: GameTheme.textSecondary, fontSize: 13),
                  ),
                ),
            ],
          ),
        ),
      ),
    );
  }

  void _showMoreMenu(BuildContext context) {
    showModalBottomSheet(
      context: context,
      backgroundColor: GameTheme.tableBlueDark,
      shape: const RoundedRectangleBorder(
        borderRadius: BorderRadius.vertical(top: Radius.circular(16)),
      ),
      builder: (ctx) => SafeArea(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            ListTile(
              leading: const Icon(Icons.help_outline, color: Colors.white70),
              title: const Text('游戏帮助', style: TextStyle(color: Colors.white)),
              onTap: () => Navigator.pop(ctx),
            ),
            ListTile(
              leading: const Icon(Icons.settings, color: Colors.white70),
              title: const Text('设置', style: TextStyle(color: Colors.white)),
              onTap: () => Navigator.pop(ctx),
            ),
            ListTile(
              leading: const Icon(Icons.exit_to_app, color: Colors.white70),
              title: const Text('退出房间', style: TextStyle(color: Colors.white)),
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
