import 'package:flutter/material.dart';
import '../config/ui_scale.dart';
import '../models/player.dart';
import '../theme/game_theme.dart';

class RoomPlayerWidget extends StatelessWidget {
  final Player? player;
  final int seatIndex;
  final double? cellWidth;
  final double? cellHeight;

  const RoomPlayerWidget({
    super.key,
    required this.player,
    this.cellWidth,
    this.cellHeight,
  }) : seatIndex = 0;

  const RoomPlayerWidget.empty({
    super.key,
    required this.seatIndex,
    this.cellWidth,
    this.cellHeight,
  }) : player = null;

  @override
  Widget build(BuildContext context) {
    final ui = context.ui;
    final h = cellHeight ?? ui.h(56);
    final w = cellWidth ?? double.infinity;
    final radius = h * 0.12;
    final avatarR = h * 0.28;
    final fontMd = h * 0.22;
    final fontSm = h * 0.18;

    if (player == null) {
      return Container(
        width: w,
        height: h,
        decoration: BoxDecoration(
          border: Border.all(color: Colors.white.withValues(alpha: 0.2)),
          borderRadius: BorderRadius.circular(radius),
          color: Colors.white.withValues(alpha: 0.05),
        ),
        child: Center(
          child: Text(
            '座位 ${seatIndex + 1}',
            style: TextStyle(color: GameTheme.textSecondary, fontSize: fontMd),
          ),
        ),
      );
    }

    return Container(
      width: w,
      height: h,
      padding: EdgeInsets.symmetric(horizontal: w * 0.06, vertical: h * 0.08),
      decoration: BoxDecoration(
        color: player!.isReady
            ? Colors.green.withValues(alpha: 0.25)
            : Colors.white.withValues(alpha: 0.08),
        borderRadius: BorderRadius.circular(radius),
        border: Border.all(
          color: player!.isReady ? Colors.greenAccent : Colors.white24,
        ),
      ),
      child: Row(
        children: [
          CircleAvatar(
            radius: avatarR,
            backgroundColor: GameTheme.tableBlueLight,
            child: Text(
              player!.nickname.isNotEmpty ? player!.nickname[0] : '?',
              style: TextStyle(color: Colors.white, fontSize: avatarR * 0.9),
            ),
          ),
          SizedBox(width: w * 0.04),
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
                    fontSize: fontMd,
                  ),
                  overflow: TextOverflow.ellipsis,
                ),
                Text(
                  player!.isReady ? '已准备' : '未准备',
                  style: TextStyle(
                    color: player!.isReady ? Colors.greenAccent : GameTheme.textSecondary,
                    fontSize: fontSm,
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
