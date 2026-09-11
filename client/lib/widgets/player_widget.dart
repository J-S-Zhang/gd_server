import 'package:flutter/material.dart';
import '../config/ui_config.dart';
import '../config/ui_scale.dart';
import 'game/game_layout_positioned.dart';
import 'game/region_child_stack.dart';
import '../models/player.dart';
import '../theme/game_theme.dart';
import '../utils/seat_layout.dart';
import 'game/finish_rank_badge.dart';
import 'game/seat_chat_bubble.dart';

const int kCardCountRevealThreshold = 10;

class PlayerWidget extends StatelessWidget {
  final Player player;
  final bool isCurrentTurn;
  final bool compact;
  final bool showLobbyState;
  final int? cardCountOverride;
  final PlayerNicknamePlacement nicknamePlacement;
  final FinishRankPlacement finishRankPlacement;
  final ChatBubblePlacement chatBubblePlacement;
  final String? chatBubble;
  final bool chatIsEmoji;
  final int maxPlayers;
  /// 对应 [seatLayout] 中当前人数档位的 seat_N，用于 w/h 比例定尺寸。
  final String? layoutElementId;

  const PlayerWidget({
    super.key,
    required this.player,
    this.isCurrentTurn = false,
    this.compact = false,
    this.showLobbyState = false,
    this.cardCountOverride,
    this.nicknamePlacement = PlayerNicknamePlacement.below,
    this.finishRankPlacement = FinishRankPlacement.below,
    this.chatBubblePlacement = ChatBubblePlacement.above,
    this.chatBubble,
    this.chatIsEmoji = false,
    this.maxPlayers = 6,
    this.layoutElementId,
  });

  int get _effectiveCardCount => cardCountOverride ?? player.cardCount;

  bool get _showCardCountBadge =>
      !showLobbyState &&
      !player.hasFinished &&
      _effectiveCardCount > 0 &&
      _effectiveCardCount <= kCardCountRevealThreshold;

  bool get _inGameInfoOnly => !showLobbyState && !compact;

  GamePageElementLayout? _layoutElement(UiScale ui) {
    if (layoutElementId == null) return null;
    final seatMatch = RegExp(r'^seat_(\d+)$').firstMatch(layoutElementId!);
    if (seatMatch != null) {
      return ui.seatLayoutElement(int.parse(seatMatch.group(1)!), maxPlayers);
    }
    return ui.config.gamePageLayout.element(layoutElementId!);
  }

  GameLayoutRect? _layoutRegion(UiScale ui) {
    if (layoutElementId == null) return null;
    if (RegExp(r'^seat_\d+$').hasMatch(layoutElementId!)) {
      return ui.layoutRegionRect(layoutElementId!, maxPlayers: maxPlayers);
    }
    return ui.elementRect(layoutElementId!);
  }

  bool _usesChildRegions(UiScale ui) {
    final element = _layoutElement(ui);
    return element?.child('avatar') != null && element?.child('nickname') != null;
  }

  @override
  Widget build(BuildContext context) {
    final ui = context.ui;
    if (_inGameInfoOnly) {
      return _buildInGameInfo(ui);
    }
    return _buildLobbyInfo(ui);
  }

  Widget _buildInGameInfo(UiScale ui) {
    final cfg = ui.config.player;
    final region = _layoutRegion(ui);
    if (region != null && _usesChildRegions(ui)) {
      return _buildConfiguredInGame(ui, region);
    }
    final avatarRadius = region != null
        ? region.height * 0.34
        : ui.r(cfg.compactAvatarRadius);
    final borderRadius = region != null ? region.height * 0.12 : ui.r(cfg.borderRadius);
    final nicknameStyle = TextStyle(
      color: GameTheme.textPrimary,
      fontSize: ui.sp(ui.config.font.sm2),
    );
    final finishLabel = player.hasFinished
        ? SeatLayout.finishRankLabel(player.finishRank, maxPlayers)
        : '';

    final avatar = Stack(
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
              fontSize: avatarRadius * 0.85,
              fontWeight: FontWeight.bold,
            ),
          ),
        ),
        if (_showCardCountBadge)
          Positioned(
            right: -avatarRadius * 0.25,
            bottom: -avatarRadius * 0.25,
            child: _CardCountBadge(
              count: _effectiveCardCount,
              ui: ui,
              compact: true,
              badgeSize: avatarRadius * 0.75,
            ),
          ),
        if (isCurrentTurn)
          Positioned(
            right: -avatarRadius * 0.15,
            top: -avatarRadius * 0.15,
            child: Container(
              width: avatarRadius * 0.35,
              height: avatarRadius * 0.35,
              decoration: const BoxDecoration(
                color: GameTheme.accentGold,
                shape: BoxShape.circle,
              ),
            ),
          ),
      ],
    );

    final nickname = ConstrainedBox(
      constraints: BoxConstraints(maxWidth: region?.width ?? ui.w(cfg.width)),
      child: Text(
        player.nickname,
        style: nicknameStyle.copyWith(
          fontSize: region != null ? region.height * 0.16 : nicknameStyle.fontSize,
        ),
        overflow: TextOverflow.ellipsis,
        maxLines: 1,
        textAlign:
            nicknamePlacement == PlayerNicknamePlacement.trailing ? TextAlign.left : TextAlign.center,
      ),
    );

    final gapW = SizedBox(width: ui.w(ui.config.spacing.sm));
    final gapH = SizedBox(height: ui.h(ui.config.spacing.xs));
    final rankBadge = finishLabel.isNotEmpty ? FinishRankBadge(label: finishLabel) : null;

    final avatarBlock = _wrapAvatarWithChatBubble(
      avatar: avatar,
      gapW: gapW,
      gapH: gapH,
      chatMaxWidth: region != null ? region.width * 0.95 : null,
    );

    Widget content;
    if (rankBadge != null) {
      switch (finishRankPlacement) {
        case FinishRankPlacement.leading:
          content = Row(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.center,
            children: [rankBadge, gapW, avatarBlock],
          );
          break;
        case FinishRankPlacement.trailing:
          content = Row(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.center,
            children: [avatarBlock, gapW, rankBadge],
          );
          break;
        case FinishRankPlacement.above:
          content = Column(
            mainAxisSize: MainAxisSize.min,
            children: [rankBadge, gapH, avatarBlock],
          );
          break;
        case FinishRankPlacement.below:
          content = Column(
            mainAxisSize: MainAxisSize.min,
            children: [avatarBlock, gapH, rankBadge],
          );
          break;
      }
    } else if (nicknamePlacement == PlayerNicknamePlacement.trailing) {
      content = Row(
        mainAxisSize: MainAxisSize.min,
        crossAxisAlignment: CrossAxisAlignment.center,
        children: [avatarBlock, gapW, nickname],
      );
    } else {
      content = Column(
        mainAxisSize: MainAxisSize.min,
        children: [avatarBlock, gapH, nickname],
      );
    }

    return Container(
      padding: ui.edgeInsetsSymmetric(horizontal: 6, vertical: 4),
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
      child: content,
    );
  }

  Widget _wrapAvatarWithChatBubble({
    required Widget avatar,
    required Widget gapW,
    required Widget gapH,
    double? chatMaxWidth,
  }) {
    final bubbleText = chatBubble;
    if (bubbleText == null || bubbleText.isEmpty) return avatar;

    final bubble = SeatChatBubbleWidget(
      content: bubbleText,
      placement: chatBubblePlacement,
      isEmoji: chatIsEmoji,
      maxWidth: chatMaxWidth,
    );

    switch (chatBubblePlacement) {
      case ChatBubblePlacement.leading:
        return Row(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.center,
          children: [bubble, gapW, avatar],
        );
      case ChatBubblePlacement.trailing:
        return Row(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.center,
          children: [avatar, gapW, bubble],
        );
      case ChatBubblePlacement.above:
        return Column(
          mainAxisSize: MainAxisSize.min,
          children: [bubble, gapH, avatar],
        );
      case ChatBubblePlacement.below:
        return Column(
          mainAxisSize: MainAxisSize.min,
          children: [avatar, gapH, bubble],
        );
    }
  }

  Widget _buildConfiguredInGame(UiScale ui, GameLayoutRect region) {
    final parentId = layoutElementId!;
    final borderRadius = region.height * 0.12;
    final avatarRect = ui.elementChildRect(parentId, 'avatar', maxPlayers: maxPlayers);
    final nicknameRect = ui.elementChildRect(parentId, 'nickname', maxPlayers: maxPlayers);
    final avatarRadius = avatarRect != null
        ? (avatarRect.width < avatarRect.height ? avatarRect.width : avatarRect.height) * 0.42
        : ui.r(ui.config.player.compactAvatarRadius);
    final nicknameFontSize = nicknameRect?.height != null
        ? nicknameRect!.height * 0.42
        : ui.sp(ui.config.font.sm2);
    final finishLabel = player.hasFinished
        ? SeatLayout.finishRankLabel(player.finishRank, maxPlayers)
        : '';

    final avatar = _buildAvatarCircle(ui, avatarRadius);
    final avatarBlock = _wrapAvatarWithChatBubble(
      avatar: avatar,
      gapW: SizedBox(width: ui.w(ui.config.spacing.sm)),
      gapH: SizedBox(height: ui.h(ui.config.spacing.xs)),
      chatMaxWidth: avatarRect?.width ?? region.width * 0.95,
    );

    return SizedBox(
      width: region.width,
      height: region.height,
      child: DecoratedBox(
        decoration: _seatDecoration(ui, borderRadius),
        child: Stack(
          clipBehavior: Clip.none,
          children: [
            RegionChildPositioned(
              parentId: parentId,
              childId: 'avatar',
              maxPlayers: maxPlayers,
              child: Stack(
                clipBehavior: Clip.none,
                alignment: Alignment.center,
                children: [
                  Center(child: avatarBlock),
                  if (finishLabel.isNotEmpty)
                    Align(
                      alignment: Alignment.topCenter,
                      child: FinishRankBadge(label: finishLabel),
                    ),
                ],
              ),
            ),
            RegionChildPositioned(
              parentId: parentId,
              childId: 'nickname',
              maxPlayers: maxPlayers,
              child: Center(
                child: Text(
                  player.nickname,
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                  textAlign: TextAlign.center,
                  style: TextStyle(
                    color: GameTheme.textPrimary,
                    fontSize: nicknameFontSize,
                    fontWeight: FontWeight.w600,
                  ),
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildConfiguredLobby(UiScale ui, GameLayoutRect region) {
    final parentId = layoutElementId!;
    final borderRadius = region.height * 0.12;
    final avatarRect = ui.elementChildRect(parentId, 'avatar', maxPlayers: maxPlayers);
    final nicknameRect = ui.elementChildRect(parentId, 'nickname', maxPlayers: maxPlayers);
    final avatarRadius = avatarRect != null
        ? (avatarRect.width < avatarRect.height ? avatarRect.width : avatarRect.height) * 0.42
        : ui.r(compact ? ui.config.player.compactAvatarRadius : ui.config.player.avatarRadius);
    final nicknameFontSize = nicknameRect?.height != null
        ? nicknameRect!.height * 0.28
        : ui.sp(ui.config.font.sm2);
    final statusText = _buildStatusText();

    return SizedBox(
      width: region.width,
      height: region.height,
      child: DecoratedBox(
        decoration: _seatDecoration(ui, borderRadius),
        child: Stack(
          clipBehavior: Clip.none,
          children: [
            RegionChildPositioned(
              parentId: parentId,
              childId: 'avatar',
              maxPlayers: maxPlayers,
              child: Center(child: _buildAvatarCircle(ui, avatarRadius)),
            ),
            RegionChildPositioned(
              parentId: parentId,
              childId: 'nickname',
              maxPlayers: maxPlayers,
              child: Center(
                child: Column(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Text(
                      player.nickname,
                      maxLines: 1,
                      overflow: TextOverflow.ellipsis,
                      textAlign: TextAlign.center,
                      style: TextStyle(
                        color: GameTheme.textPrimary,
                        fontSize: nicknameFontSize,
                        fontWeight: FontWeight.w600,
                      ),
                    ),
                    if (player.isBot)
                      Text(
                        '机器人',
                        style: TextStyle(
                          color: Colors.cyanAccent,
                          fontSize: nicknameFontSize * 0.75,
                        ),
                      ),
                    if (statusText != null)
                      Text(
                        statusText,
                        style: TextStyle(
                          color: player.hasFinished
                              ? GameTheme.accentGold
                              : showLobbyState && player.isReady
                                  ? Colors.greenAccent
                                  : GameTheme.textSecondary,
                          fontSize: nicknameFontSize * 0.8,
                          fontWeight: FontWeight.w600,
                        ),
                      ),
                  ],
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }

  BoxDecoration _seatDecoration(UiScale ui, double borderRadius) {
    return BoxDecoration(
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
    );
  }

  Widget _buildAvatarCircle(UiScale ui, double avatarRadius) {
    return Stack(
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
              fontSize: avatarRadius * 0.85,
              fontWeight: FontWeight.bold,
            ),
          ),
        ),
        if (_showCardCountBadge)
          Positioned(
            right: -avatarRadius * 0.25,
            bottom: -avatarRadius * 0.25,
            child: _CardCountBadge(
              count: _effectiveCardCount,
              ui: ui,
              compact: true,
              badgeSize: avatarRadius * 0.75,
            ),
          ),
        if (isCurrentTurn)
          Positioned(
            right: -avatarRadius * 0.15,
            top: -avatarRadius * 0.15,
            child: Container(
              width: avatarRadius * 0.35,
              height: avatarRadius * 0.35,
              decoration: const BoxDecoration(
                color: GameTheme.accentGold,
                shape: BoxShape.circle,
              ),
            ),
          ),
      ],
    );
  }

  Widget _buildLobbyInfo(UiScale ui) {
    final cfg = ui.config.player;
    final region = _layoutRegion(ui);
    if (region != null && _usesChildRegions(ui)) {
      return _buildConfiguredLobby(ui, region);
    }
    final level = (player.id % 15) + 5;
    final coins = _formatCoins((player.id * 1379) % 99999 + 1000);
    final width = region?.width ?? ui.w(compact ? cfg.compactWidth : cfg.width);
    final avatarRadius = region != null
        ? region.height * (compact ? 0.34 : 0.3)
        : ui.r(compact ? cfg.compactAvatarRadius : cfg.avatarRadius);
    final borderRadius = region != null ? region.height * 0.12 : ui.r(cfg.borderRadius);
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
  final double? badgeSize;

  const _CardCountBadge({
    required this.count,
    required this.ui,
    required this.compact,
    this.badgeSize,
  });

  @override
  Widget build(BuildContext context) {
    final size = badgeSize ?? ui.r(compact ? 18 : 22);
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
