import 'dart:math' as math;

import 'package:flutter/material.dart';
import '../config/ui_scale.dart';
import 'game/game_layout_positioned.dart';
import '../models/card.dart';
import '../utils/hand_layout.dart';
import 'poker_card.dart';

class HandCardsWidget extends StatefulWidget {
  final List<GameCard> cards;
  final int currentLevel;
  final List<Set<int>> organizedGroups;
  final void Function(int cardId) onCardTap;
  final ValueChanged<double>? onRowHeightChanged;
  final bool readOnly;

  const HandCardsWidget({
    super.key,
    required this.cards,
    required this.currentLevel,
    this.organizedGroups = const [],
    required this.onCardTap,
    this.onRowHeightChanged,
    this.readOnly = false,
  });

  @override
  State<HandCardsWidget> createState() => _HandCardsWidgetState();
}

class _HandCardsWidgetState extends State<HandCardsWidget> {
  double? _lastReportedHeight;

  void _reportRowHeight(double height) {
    if (_lastReportedHeight == height) return;
    _lastReportedHeight = height;
    widget.onRowHeightChanged?.call(height);
  }

  @override
  Widget build(BuildContext context) {
    final ui = context.ui;
    final handCfg = ui.config.handCards;
    final baseRegion = ui.elementRect('hand_cards');
    final cardSize = ui.handCardSizePx();
    final cardWidth = cardSize.width;
    final cardHeight = cardSize.height;
    final hStep = cardWidth * (1 - handCfg.horizontalOverlap);
    final vStep = cardHeight * (1 - handCfg.verticalOverlap);
    final oneRowHeight = baseRegion?.height ?? ui.h(handCfg.height);
    final selectionLift = baseRegion != null
        ? baseRegion.height * (handCfg.selectionLift / handCfg.height)
        : ui.h(handCfg.selectionLift);
    final extraPadding = baseRegion != null
        ? baseRegion.height * (handCfg.extraPadding / handCfg.height)
        : ui.h(handCfg.extraPadding);

    final rowHeight = computeHandCardsRowHeight(
      cards: widget.cards,
      currentLevel: widget.currentLevel,
      organizedGroups: widget.organizedGroups,
      cardHeight: cardHeight,
      verticalOverlap: handCfg.verticalOverlap,
      selectionLift: selectionLift,
      emptyPlaceholderHeight: oneRowHeight,
      extraPadding: extraPadding,
    );

    final regionHeight = math.max(oneRowHeight, rowHeight);
    WidgetsBinding.instance.addPostFrameCallback((_) => _reportRowHeight(rowHeight));

    if (widget.cards.isEmpty) {
      return SizedBox(
        height: rowHeight,
        child: Center(
          child: Text(
            '等待发牌...',
            style: TextStyle(color: Colors.white54, fontSize: ui.sp(ui.config.font.md)),
          ),
        ),
      );
    }

    final groups = buildHandDisplayGroups(
      widget.cards,
      currentLevel: widget.currentLevel,
      organizedGroups: widget.organizedGroups,
    );

    final contentWidth = cardWidth + (groups.length - 1) * hStep;
    final contentHeight = rowHeight - extraPadding;

    final handStack = SizedBox(
      width: contentWidth,
      height: contentHeight,
      child: Stack(
        clipBehavior: Clip.none,
        children: [
          for (var gi = 0; gi < groups.length; gi++)
            for (var i = groups[gi].length - 1; i >= 0; i--)
              Positioned(
                left: gi * hStep,
                bottom: i * vStep,
                child: PokerCardWidget(
                  card: groups[gi][i],
                  width: cardWidth,
                  height: cardHeight,
                  currentLevel: widget.currentLevel,
                  onTap: widget.readOnly
                      ? null
                      : () => widget.onCardTap(groups[gi][i].id),
                ),
              ),
        ],
      ),
    );

    return SizedBox(
      height: regionHeight,
      child: LayoutBuilder(
        builder: (context, constraints) {
          return SingleChildScrollView(
            scrollDirection: Axis.horizontal,
            padding: ui.edgeInsetsSymmetric(horizontal: handCfg.scrollPaddingHorizontal),
            child: ConstrainedBox(
              constraints: BoxConstraints(
                minWidth: constraints.maxWidth,
                minHeight: constraints.maxHeight,
              ),
              child: Align(
                alignment: Alignment.bottomCenter,
                child: handStack,
              ),
            ),
          );
        },
      ),
    );
  }
}
