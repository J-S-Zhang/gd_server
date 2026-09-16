import 'package:flutter/material.dart';
import '../config/ui_scale.dart';
import '../models/player.dart';
import '../theme/game_theme.dart';
import 'player_avatar.dart';

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
    final avatarSize = h * 0.42;
    final fontMd = h * 0.18;

    if (player == null) {
      return SizedBox(
        width: w,
        height: h,
        child: Center(
          child: Text(
            '座位 ${seatIndex + 1}',
            style: TextStyle(color: GameTheme.textSecondary, fontSize: fontMd),
          ),
        ),
      );
    }

    return SizedBox(
      width: w,
      height: h,
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          PlayerAvatar(
            nickname: player!.nickname,
            avatarPresetId: player!.avatarPreset,
            avatarPath: player!.avatar,
            radius: avatarSize / 2,
            clipCircle: false,
          ),
          SizedBox(height: h * 0.06),
          Text(
            player!.nickname,
            style: TextStyle(
              fontWeight: FontWeight.bold,
              color: Colors.white,
              fontSize: fontMd,
            ),
            overflow: TextOverflow.ellipsis,
            maxLines: 1,
            textAlign: TextAlign.center,
          ),
        ],
      ),
    );
  }
}
