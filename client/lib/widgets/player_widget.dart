import 'package:flutter/material.dart';
import '../config/ui_scale.dart';
import '../models/player.dart';
import '../theme/game_theme.dart';

const int kCardCountRevealThreshold = 10;

class PlayerWidget extends StatelessWidget {
  final Player player;
  final bool isCurrentTurn;
  final bool compact;
  final bool showLobbyState;
  final int? cardCountOverride;

  const PlayerWidget({
    super.key,
    required this.player,
    this.isCurrentTurn = false,
    this.compact = false,
    this.showLobbyState = false,
    this.cardCountOverride,
  });

  int get _effectiveCardCount => cardCountOverride ?? player.cardCount;

  bool get _showCardCountBadge =>
      !showLobbyState &&
      !player.hasFinished &&
      _effectiveCardCount > 0 &&
      _effectiveCardCount <= kCardCountRevealThreshold;

  @override
  Widget build(BuildContext context) {
    final ui = context.ui;
    final cfg = ui.config.player;
    final level = (player.id % 15) + 5;
    final coins = _formatCoins((player.id * 1379) % 99999 + 1000);
    final width = ui.w(compact ? cfg.compactWidth : cfg.width);
    final avatarRadius = ui.r(compact ? cfg.compactAvatarRadius : cfg.avatarRadius);
    final borderRadius = ui.r(cfg.borderRadius);
    final statusText = _buildStatusText();

    return Container(
      width: width,
      padding: ui.edgeInsetsSymmetric(
        horizontal: compact ? 4 : 8,
        vertical: compact ? 4 : 6,
      ),
      decoration: BoxDecoration(
        color: isCurrentTurn
            ? GameTheme.accentGold.withValues(alpha: 0.25)
            : Colors.black.withValues(alpha: 0.35),
        borderRadius: BorderRadius.circular(borderRadius),
        border: Border.all(
          color: isCurrentTurn ? GameTheme.accentGold : Colors.white.withValues(alpha: 0.15),
          width: isCurrentTurn ? ui.r(2) : ui.r(1),
        ),
        boxShadow: isCurrentTurn
            ? [
                BoxShadow(
                  color: GameTheme.accentGold.withValues(alpha: 0.35),
                  blurRadius: ui.r(8),
                ),
              ]
            : null,
      ),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Stack(
            clipBehavior: Clip.none,
            children: [
              CircleAvatar(
                radius: avatarRadius,
                backgroundColor:
                    player.team == 0 ? GameTheme.tableBlueLight : const Color(0xFFE53935),
                child: Text(
                  player.nickname.isNotEmpty ? player.nickname[0] : '?',
                  style: TextStyle(
                    color: Colors.white,
                    fontSize: ui.sp(compact ? ui.config.font.lg : ui.config.font.lg),
                    fontWeight: FontWeight.bold,
                  ),
                ),
              ),
              if (_showCardCountBadge)
                Positioned(
                  right: ui.w(-6),
                  bottom: ui.h(-6),
                  child: _CardCountBadge(count: _effectiveCardCount, ui: ui, compact: compact),
                ),
              if (isCurrentTurn)
                Positioned(
                  right: ui.w(-4),
                  top: ui.h(-4),
                  child: Container(
                    width: ui.w(10),
                    height: ui.h(10),
                    decoration: const BoxDecoration(
                      color: GameTheme.accentGold,
                      shape: BoxShape.circle,
                    ),
                  ),
                ),
            ],
          ),
          SizedBox(height: ui.h(compact ? 2 : 4)),
          Text(
            player.nickname,
            style: TextStyle(color: GameTheme.textPrimary, fontSize: ui.sp(ui.config.font.sm2)),
            overflow: TextOverflow.ellipsis,
            maxLines: 1,
          ),
          if (player.isBot)
            Text(
              '机器人',
              style: TextStyle(color: Colors.cyanAccent, fontSize: ui.sp(ui.config.font.xs)),
            ),
          if (!compact) ...[
            Text(
              'LV$level',
              style: TextStyle(color: GameTheme.textSecondary, fontSize: ui.sp(ui.config.font.sm)),
            ),
            Text(
              coins,
              style: TextStyle(color: GameTheme.accentGold, fontSize: ui.sp(ui.config.font.sm)),
            ),
          ],
          if (statusText != null)
            Text(
              statusText,
              style: TextStyle(
                color: player.hasFinished
                    ? GameTheme.accentGold
                    : showLobbyState && player.isReady
                        ? Colors.greenAccent
                        : GameTheme.textSecondary,
                fontSize: ui.sp(ui.config.font.sm),
                fontWeight: FontWeight.w600,
              ),
            ),
        ],
      ),
    );
  }

  String? _buildStatusText() {
    if (player.hasFinished) {
      return '第${player.finishRank}名';
    }
    if (showLobbyState) {
      return player.isReady ? '已准备' : '未准备';
    }
    return null;
  }

  String _formatCoins(int value) {
    if (value >= 10000) {
      return '${(value / 10000).toStringAsFixed(2)}万';
    }
    return value.toString();
  }
}

class _CardCountBadge extends StatelessWidget {
  final int count;
  final UiScale ui;
  final bool compact;

  const _CardCountBadge({
    required this.count,
    required this.ui,
    required this.compact,
  });

  @override
  Widget build(BuildContext context) {
    final size = ui.r(compact ? 18 : 22);
    return Container(
      width: size,
      height: size,
      alignment: Alignment.center,
      decoration: BoxDecoration(
        color: const Color(0xFFE53935),
        shape: BoxShape.circle,
        border: Border.all(color: Colors.white, width: ui.r(1.5)),
        boxShadow: [
          BoxShadow(
            color: Colors.black.withValues(alpha: 0.35),
            blurRadius: ui.r(4),
          ),
        ],
      ),
      child: Text(
        '$count',
        style: TextStyle(
          color: Colors.white,
          fontSize: ui.sp(compact ? ui.config.font.xs : ui.config.font.sm),
          fontWeight: FontWeight.bold,
          height: 1,
        ),
      ),
    );
  }
}
