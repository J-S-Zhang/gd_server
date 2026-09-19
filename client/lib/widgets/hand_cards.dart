import 'package:flutter/material.dart';
import '../config/ui_scale.dart';
import '../models/card.dart';
import '../utils/hand_layout.dart';
import 'game/game_layout_positioned.dart';
import 'poker_card.dart';

class HandCardsWidget extends StatefulWidget {
  final List<GameCard> cards;
  final int currentLevel;
  final List<Set<int>> organizedGroups;
  final void Function(int cardId) onCardTap;
  final void Function(Set<int> cardIds)? onBoxSelect;
  final bool readOnly;

  const HandCardsWidget({
    super.key,
    required this.cards,
    required this.currentLevel,
    this.organizedGroups = const [],
    required this.onCardTap,
    this.onBoxSelect,
    this.readOnly = false,
  });

  @override
  State<HandCardsWidget> createState() => _HandCardsWidgetState();
}

class _HandCardHitTarget {
  const _HandCardHitTarget({required this.card, required this.rect});

  final GameCard card;
  final Rect rect;
}

class _HandCardsWidgetState extends State<HandCardsWidget> {
  static const _layoutAnimDuration = Duration(milliseconds: 220);
  static const _layoutAnimCurve = Curves.easeOutCubic;
  static const _dragThreshold = 10.0;

  Offset? _dragStart;
  Offset? _dragCurrent;
  bool _isBoxDragging = false;
  Set<int> _previewSelectedIds = {};

  @override
  Widget build(BuildContext context) {
    final ui = context.ui;
    final handCfg = ui.config.handCards;
    final cardSize = ui.handCardSizePx();
    final cardWidth = cardSize.width;
    final cardHeight = cardSize.height;
    final double hStep = cardWidth * (1 - handCfg.horizontalOverlap);
    final double vStep = cardHeight * (1 - handCfg.verticalOverlap);

    if (widget.cards.isEmpty) {
      return const SizedBox.shrink();
    }

    final contentHeight = ui.computeHandCardsContentHeight(
      cards: widget.cards,
      currentLevel: widget.currentLevel,
      organizedGroups: widget.organizedGroups,
    );
    final selectionLift = ui.handCardsSelectionLiftPx;
    final regionContentHeight = ui.computeHandCardsRegionHeight(
      cards: widget.cards,
      currentLevel: widget.currentLevel,
      organizedGroups: widget.organizedGroups,
    );

    final groups = buildHandDisplayGroups(
      widget.cards,
      currentLevel: widget.currentLevel,
      organizedGroups: widget.organizedGroups,
    );

    final contentWidth = cardWidth + (groups.length - 1) * hStep;

    final hitTargets = _buildHitTargets(
      groups: groups,
      cardWidth: cardWidth,
      cardHeight: cardHeight,
      hStep: hStep,
      vStep: vStep,
      contentHeight: contentHeight,
    );

    Rect? marqueeRect;
    if (_dragStart != null && _dragCurrent != null && _isBoxDragging) {
      marqueeRect = Rect.fromPoints(_dragStart!, _dragCurrent!);
    }

    return LayoutBuilder(
      builder: (context, constraints) {
        final regionHeight = constraints.maxHeight.isFinite && constraints.maxHeight > 0
            ? constraints.maxHeight
            : regionContentHeight;
        return SizedBox(
          height: regionHeight,
          child: SingleChildScrollView(
            scrollDirection: Axis.horizontal,
            physics: _isBoxDragging
                ? const NeverScrollableScrollPhysics()
                : const BouncingScrollPhysics(),
            padding: ui.edgeInsetsSymmetric(
              horizontal: ui.canvasFracW(handCfg.scrollPaddingHorizontalRatio),
            ),
            child: ConstrainedBox(
              constraints: BoxConstraints(
                minWidth: constraints.maxWidth,
                minHeight: regionHeight,
              ),
              child: Padding(
                padding: EdgeInsets.only(top: selectionLift),
                child: Align(
                  alignment: Alignment.bottomCenter,
                  child: Listener(
                  behavior: HitTestBehavior.translucent,
                  onPointerDown: widget.readOnly ? null : (e) => _onPointerDown(e.localPosition),
                  onPointerMove: widget.readOnly ? null : (e) => _onPointerMove(e.localPosition, hitTargets),
                  onPointerUp: widget.readOnly
                      ? null
                      : (e) => _onPointerUp(e.localPosition, hitTargets),
                  onPointerCancel: widget.readOnly ? null : (_) => _resetDrag(),
                  child: AnimatedContainer(
                    duration: _layoutAnimDuration,
                    curve: _layoutAnimCurve,
                    width: contentWidth,
                    height: contentHeight,
                    child: Stack(
                      clipBehavior: Clip.none,
                      alignment: Alignment.bottomLeft,
                      children: [
                        for (var gi = 0; gi < groups.length; gi++)
                          AnimatedPositioned(
                            key: ValueKey(_groupKey(groups[gi])),
                            duration: _layoutAnimDuration,
                            curve: _layoutAnimCurve,
                            left: gi.toDouble() * hStep,
                            bottom: 0,
                            width: cardWidth,
                            height: cardHeight + (groups[gi].length - 1).toDouble() * vStep,
                            child: Stack(
                              clipBehavior: Clip.none,
                              alignment: Alignment.bottomCenter,
                              children: [
                                // 先画上方的牌（底层），最后画最下面的牌（顶层），避免遮挡点数。
                                for (var i = groups[gi].length - 1; i >= 0; i--)
                                  Positioned(
                                    bottom: i.toDouble() * vStep,
                                    child: PokerCardWidget(
                                      card: groups[gi][i],
                                      width: cardWidth,
                                      height: cardHeight,
                                      currentLevel: widget.currentLevel,
                                      previewSelected: _previewSelectedIds.contains(groups[gi][i].id),
                                    ),
                                  ),
                              ],
                            ),
                          ),
                        if (marqueeRect != null)
                          Positioned(
                            left: marqueeRect.left,
                            top: marqueeRect.top,
                            width: marqueeRect.width,
                            height: marqueeRect.height,
                            child: IgnorePointer(
                              child: DecoratedBox(
                                decoration: BoxDecoration(
                                  color: const Color(0xFF64B5F6).withValues(alpha: 0.12),
                                  border: Border.all(
                                    color: const Color(0xFF64B5F6).withValues(alpha: 0.85),
                                    width: 1.5,
                                  ),
                                ),
                              ),
                            ),
                          ),
                      ],
                    ),
                  ),
                ),
              ),
            ),
          ),
        ),
        );
      },
    );
  }

  void _onPointerDown(Offset position) {
    _dragStart = position;
    _dragCurrent = position;
    _isBoxDragging = false;
    _previewSelectedIds = {};
  }

  void _onPointerMove(Offset position, List<_HandCardHitTarget> hitTargets) {
    if (_dragStart == null) return;
    _dragCurrent = position;
    final moved = (position - _dragStart!).distance;
    if (!_isBoxDragging && moved < _dragThreshold) return;

    _isBoxDragging = true;
    final rect = Rect.fromPoints(_dragStart!, _dragCurrent!);
    final ids = _cardIdsInRect(rect, hitTargets);
    setState(() {
      _previewSelectedIds = ids;
    });
  }

  void _onPointerUp(Offset position, List<_HandCardHitTarget> hitTargets) {
    if (_dragStart == null) {
      _resetDrag();
      return;
    }

    if (_isBoxDragging) {
      final rect = Rect.fromPoints(_dragStart!, _dragCurrent ?? position);
      final ids = _cardIdsInRect(rect, hitTargets);
      if (ids.isNotEmpty) {
        widget.onBoxSelect?.call(ids);
      }
      _resetDrag();
      return;
    }

    final tapped = _cardIdAt(_dragStart!, hitTargets);
    if (tapped != null) {
      widget.onCardTap(tapped);
    }
    _resetDrag();
  }

  void _resetDrag() {
    if (!_isBoxDragging && _previewSelectedIds.isEmpty && _dragStart == null) {
      return;
    }
    setState(() {
      _dragStart = null;
      _dragCurrent = null;
      _isBoxDragging = false;
      _previewSelectedIds = {};
    });
  }

  List<_HandCardHitTarget> _buildHitTargets({
    required List<List<GameCard>> groups,
    required double cardWidth,
    required double cardHeight,
    required double hStep,
    required double vStep,
    required double contentHeight,
  }) {
    final targets = <_HandCardHitTarget>[];
    for (var gi = 0; gi < groups.length; gi++) {
      for (var i = 0; i < groups[gi].length; i++) {
        final left = gi.toDouble() * hStep;
        final top = contentHeight - cardHeight - i.toDouble() * vStep;
        targets.add(
          _HandCardHitTarget(
            card: groups[gi][i],
            rect: Rect.fromLTWH(left, top, cardWidth, cardHeight),
          ),
        );
      }
    }
    return targets;
  }

  Set<int> _cardIdsInRect(Rect rect, List<_HandCardHitTarget> hitTargets) {
    final ids = <int>{};
    for (final target in hitTargets) {
      if (rect.overlaps(target.rect)) {
        ids.add(target.card.id);
      }
    }
    return ids;
  }

  int? _cardIdAt(Offset position, List<_HandCardHitTarget> hitTargets) {
    for (final target in hitTargets) {
      if (target.rect.contains(position)) {
        return target.card.id;
      }
    }
    return null;
  }

  String _groupKey(List<GameCard> group) {
    return group.map((c) => c.id).join('_');
  }
}
