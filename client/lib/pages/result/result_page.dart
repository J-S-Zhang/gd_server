import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';
import '../../theme/game_theme.dart';

class ResultPage extends StatelessWidget {
  const ResultPage({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Container(
        decoration: const BoxDecoration(gradient: GameTheme.pageGradient),
        child: SafeArea(
          child: Center(
            child: Container(
              margin: const EdgeInsets.all(32),
              padding: const EdgeInsets.all(32),
              decoration: GameTheme.panelDecoration(),
              child: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  const Icon(Icons.emoji_events, size: 80, color: GameTheme.accentGold),
                  const SizedBox(height: 24),
                  const Text(
                    '本局结束',
                    style: TextStyle(
                      fontSize: 32,
                      color: Colors.white,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                  const SizedBox(height: 12),
                  const Text(
                    '感谢参与，期待下一局',
                    style: TextStyle(color: GameTheme.textSecondary, fontSize: 14),
                  ),
                  const SizedBox(height: 48),
                  ElevatedButton(
                    onPressed: () => context.go('/lobby'),
                    style: GameTheme.playButtonStyle.copyWith(
                      minimumSize: WidgetStateProperty.all(const Size(180, 48)),
                    ),
                    child: const Text('返回大厅', style: TextStyle(fontSize: 18)),
                  ),
                ],
              ),
            ),
          ),
        ),
      ),
    );
  }
}
