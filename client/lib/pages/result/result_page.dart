import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';
import '../../config/ui_scale.dart';
import '../../theme/game_theme.dart';

class ResultPage extends StatelessWidget {
  const ResultPage({super.key});

  @override
  Widget build(BuildContext context) {
    final ui = context.ui;

    return Scaffold(
      body: Container(
        decoration: const BoxDecoration(gradient: GameTheme.pageGradient),
        child: SafeArea(
          child: Center(
            child: Container(
              margin: ui.edgeInsetsAll(ui.config.spacing.page),
              padding: ui.edgeInsetsAll(ui.config.spacing.page),
              decoration: GameTheme.panelDecoration(ui),
              child: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Icon(
                    Icons.emoji_events,
                    size: ui.sp(80),
                    color: GameTheme.accentGold,
                  ),
                  SizedBox(height: ui.h(ui.config.spacing.xxl)),
                  Text(
                    '本局结束',
                    style: TextStyle(
                      fontSize: ui.sp(ui.config.font.title),
                      color: Colors.white,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                  SizedBox(height: ui.h(ui.config.spacing.lg)),
                  Text(
                    '感谢参与，期待下一局',
                    style: TextStyle(
                      color: GameTheme.textSecondary,
                      fontSize: ui.sp(ui.config.font.md2),
                    ),
                  ),
                  SizedBox(height: ui.h(ui.config.spacing.page + 16)),
                  ElevatedButton(
                    onPressed: () => context.go('/lobby'),
                    style: GameTheme.playButtonStyle(ui).copyWith(
                      minimumSize: WidgetStateProperty.all(
                        Size(ui.w(180), ui.h(ui.config.layout.loginButtonHeight)),
                      ),
                    ),
                    child: Text('返回大厅', style: TextStyle(fontSize: ui.sp(ui.config.font.xl))),
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
