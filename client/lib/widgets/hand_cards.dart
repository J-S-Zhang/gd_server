import 'package:flutter/material.dart';
import '../config/ui_scale.dart';
import '../models/card.dart';
import '../utils/hand_layout.dart';
import 'poker_card.dart';

class HandCardsWidget extends StatefulWidget {
  final List<GameCard> cards;
  final int currentLevel;
  final Set<int> straightStackIds;
  final void Function(int cardId) onCardTap;
  final ValueChanged<double>? onRowHeightChanged;

  const HandCardsWidget({
    super.key,
    required this.cards,
    required this.currentLevel,
    this.straightStackIds = const {},
    required this.onCardTap,
    this.onRowHeightChanged,
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
    final cardCfg = ui.config.card;
    final cardWidth = ui.w(cardCfg.handWidth);
    final cardHeight = ui.h(cardCfg.handHeight);
    final hStep = cardWidth * (1 - cardCfg.horizontalOverlap);
    final vStep = cardHeight * (1 - cardCfg.verticalOverlap);
    final selectionLift = ui.h(cardCfg.selectionLift);
    final extraPadding = ui.h(4);

    final rowHeight = computeHandCardsRowHeight(
      cards: widget.cards,
      currentLevel: widget.currentLevel,
      straightStackIds: widget.straightStackIds,
      cardHeight: cardHeight,
      verticalOverlap: cardCfg.verticalOverlap,
      selectionLift: selectionLift,
      emptyPlaceholderHeight: ui.h(90),
      extraPadding: extraPadding,
    );

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
      straightStackIds: widget.straightStackIds,
    );

    final contentWidth = cardWidth + (groups.length - 1) * hStep;
    final contentHeight = rowHeight - extraPadding;

    return SizedBox(
      height: rowHeight,
      child: SingleChildScrollView(
        scrollDirection: Axis.horizontal,
        padding: ui.edgeInsetsSymmetric(horizontal: ui.config.spacing.md),
        child: SizedBox(
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
                      onTap: () => widget.onCardTap(groups[gi][i].id),
                    ),
                  ),
            ],
          ),
        ),
      ),
    );
  }
}
