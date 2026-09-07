import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import '../../controller/game_controller.dart';
import '../../controller/room_controller.dart';
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
    // 重连后请求游戏快照
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
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [Color(0xFF1B5E20), Color(0xFF0D3311)],
          ),
        ),
        child: SafeArea(
          child: Column(
            children: [
              Padding(
                padding: const EdgeInsets.all(8),
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    Text('房间 ${widget.roomId}',
                        style: const TextStyle(color: Colors.white70)),
                    Text('打 ${gameState.currentLevel}',
                        style: const TextStyle(
                            color: Colors.amber,
                            fontSize: 18,
                            fontWeight: FontWeight.bold)),
                    if (gameState.isMyTurn)
                      CountdownWidget(
                        key: ValueKey(gameState.turnId),
                        seconds: Constants.turnTimeoutSeconds,
                      ),
                  ],
                ),
              ),
              Expanded(
                child: GameTableWidget(
                  gameState: gameState,
                  roomId: widget.roomId,
                ),
              ),
              HandCardsWidget(
                cards: gameState.myCards,
                onCardTap: controller.toggleCardSelection,
              ),
              Padding(
                padding: const EdgeInsets.all(12),
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    ElevatedButton(
                      onPressed: gameState.isMyTurn
                          ? () => controller.pass(widget.roomId)
                          : null,
                      style: ElevatedButton.styleFrom(
                        backgroundColor: Colors.grey,
                      ),
                      child: const Text('过牌'),
                    ),
                    const SizedBox(width: 24),
                    ElevatedButton(
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
