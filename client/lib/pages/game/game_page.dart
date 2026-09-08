import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import '../../controller/game_controller.dart';
import '../../controller/room_controller.dart';
import '../../models/room.dart';
import '../../models/game_state.dart';
import '../../utils/constants.dart';
import '../../widgets/game_table.dart';
import '../../widgets/hand_cards.dart';
import '../../widgets/countdown.dart';

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

  @override
  Widget build(BuildContext context) {
    final gameState = ref.watch(gameStateProvider);
    final controller = ref.read(gameControllerProvider);

    if (gameState.phase == GamePhase.finished ||
        gameState.phase == GamePhase.settlement) {
      WidgetsBinding.instance.addPostFrameCallback((_) {
        if (mounted) context.go('/result/${widget.roomId}');
      });
    }

    return Scaffold(
      body: Container(
        decoration: const BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topLeft,
            end: Alignment.bottomRight,
            colors: [Color(0xFF1B5E20), Color(0xFF0D3311)],
          ),
        ),
        child: SafeArea(
          child: Column(
            children: [
              // 顶栏
              Padding(
                padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 6),
                child: Row(
                  children: [
                    Text('房间 ${widget.roomId}',
                        style: const TextStyle(color: Colors.white70)),
                    const Spacer(),
                    Text('打 ${gameState.currentLevel}',
                        style: const TextStyle(
                            color: Colors.amber,
                            fontSize: 18,
                            fontWeight: FontWeight.bold)),
                    const SizedBox(width: 24),
                    if (gameState.isMyTurn)
                      CountdownWidget(
                        key: ValueKey(gameState.turnId),
                        seconds: Constants.turnTimeoutSeconds,
                      ),
                  ],
                ),
              ),
              // 牌桌（横屏主体）
              Expanded(
                child: GameTableWidget(
                  gameState: gameState,
                  roomId: widget.roomId,
                ),
              ),
              // 手牌 + 操作区
              Container(
                padding: const EdgeInsets.fromLTRB(12, 8, 12, 12),
                decoration: BoxDecoration(
                  color: Colors.black.withValues(alpha: 0.25),
                ),
                child: Row(
                  crossAxisAlignment: CrossAxisAlignment.end,
                  children: [
                    Expanded(
                      child: HandCardsWidget(
                        cards: gameState.myCards,
                        onCardTap: controller.toggleCardSelection,
                      ),
                    ),
                    const SizedBox(width: 16),
                    Column(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        SizedBox(
                          width: 100,
                          height: 44,
                          child: ElevatedButton(
                            onPressed: gameState.isMyTurn
                                ? () => controller.pass(widget.roomId)
                                : null,
                            style: ElevatedButton.styleFrom(
                              backgroundColor: Colors.grey.shade700,
                            ),
                            child: const Text('过牌'),
                          ),
                        ),
                        const SizedBox(height: 8),
                        SizedBox(
                          width: 100,
                          height: 44,
                          child: ElevatedButton(
                            onPressed: gameState.isMyTurn &&
                                    controller.selectedCardIds.isNotEmpty
                                ? () => controller.playCards(
                                      widget.roomId,
                                      controller.selectedCardIds,
                                    )
                                : null,
                            style: ElevatedButton.styleFrom(
                              backgroundColor: Colors.amber,
                              foregroundColor: Colors.black,
                            ),
                            child: const Text('出牌'),
                          ),
                        ),
                      ],
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
}
