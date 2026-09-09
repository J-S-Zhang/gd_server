import 'package:flutter/material.dart';
import '../config/ui_scale.dart';
import '../models/player.dart';
import '../theme/game_theme.dart';

class RoomPlayerWidget extends StatelessWidget {
  final Player? player;
  final int seatIndex;

  const RoomPlayerWidget({super.key, required this.player})
      : seatIndex = 0;

  const RoomPlayerWidget.empty({super.key, required this.seatIndex})
      : player = null;

  @override
  Widget build(BuildContext context) {
    final ui = context.ui;

    if (player == null) {
      return Container(
        decoration: BoxDecoration(
          border: Border.all(color: Colors.white.withValues(alpha: 0.2)),
          borderRadius: BorderRadius.circular(ui.r(ui.config.radius.sm)),
          color: Colors.white.withValues(alpha: 0.05),
        ),
        child: Center(
          child: Text(
            '座位 ${seatIndex + 1}',
            style: TextStyle(
              color: GameTheme.textSecondary,
              fontSize: ui.sp(ui.config.font.md2),
            ),
          ),
        ),
      );
    }

    return Container(
      padding: ui.edgeInsetsAll(ui.config.spacing.lg),
      decoration: BoxDecoration(
        color: player!.isReady
            ? Colors.green.withValues(alpha: 0.25)
            : Colors.white.withValues(alpha: 0.08),
        borderRadius: BorderRadius.circular(ui.r(ui.config.radius.sm)),
        border: Border.all(
          color: player!.isReady ? Colors.greenAccent : Colors.white24,
        ),
      ),
      child: Row(
        children: [
          CircleAvatar(
            radius: ui.r(ui.config.player.compactAvatarRadius),
            backgroundColor: GameTheme.tableBlueLight,
            child: Text(
              player!.nickname.isNotEmpty ? player!.nickname[0] : '?',
              style: TextStyle(color: Colors.white, fontSize: ui.sp(ui.config.font.md2)),
            ),
          ),
          SizedBox(width: ui.w(ui.config.spacing.lg)),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Text(
                  player!.nickname,
                  style: TextStyle(
                    fontWeight: FontWeight.bold,
                    color: Colors.white,
                    fontSize: ui.sp(ui.config.font.md2),
                  ),
                ),
                Text(
                  player!.isReady ? '已准备' : '未准备',
                  style: TextStyle(
                    color: player!.isReady ? Colors.greenAccent : GameTheme.textSecondary,
                    fontSize: ui.sp(ui.config.font.sm2 + 1),
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}
