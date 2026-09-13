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
  final bool readOnly;

  const HandCardsWidget({
    super.key,
    required this.cards,
    required this.currentLevel,
    this.organizedGroups = const [],
    required this.onCardTap,
    this.readOnly = false,
  });

  @override
  State<HandCardsWidget> createState() => _HandCardsWidgetState();
}

class _HandCardsWidgetState extends State<HandCardsWidget> {
  static const _layoutAnimDuration = Duration(milliseconds: 220);
  static const _layoutAnimCurve = Curves.easeOutCubic;

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
    final regionHeight = baseRegion?.height ?? ui.h(handCfg.height);
    final selectionLift = baseRegion != null
        ? baseRegion.height * (handCfg.selectionLift / handCfg.height)
        : ui.h(handCfg.selectionLift);
    final extraPadding = baseRegion != null
        ? baseRegion.height * (handCfg.extraPadding / handCfg.height)
        : ui.h(handCfg.extraPadding);

    if (widget.cards.isEmpty) {
      return const SizedBox.shrink();
    }

    final groups = buildHandDisplayGroups(
      widget.cards,
      currentLevel: widget.currentLevel,
      organizedGroups: widget.organizedGroups,
    );

    final contentWidth = cardWidth + (groups.length - 1) * hStep;
    final maxStackDepth = groups.fold<int>(
      1,
      (max, group) => group.length > max ? group.length : max,
    );
    final contentHeight =
        cardHeight + (maxStackDepth - 1) * vStep + selectionLift + extraPadding;

    return SizedBox(
      height: regionHeight,
      child: LayoutBuilder(
        builder: (context, constraints) {
          return SingleChildScrollView(
            scrollDirection: Axis.horizontal,
            padding: ui.edgeInsetsSymmetric(
              horizontal: handCfg.scrollPaddingHorizontal,
            ),
            child: ConstrainedBox(
              constraints: BoxConstraints(
                minWidth: constraints.maxWidth,
                minHeight: constraints.maxHeight,
              ),
              child: Align(
                alignment: Alignment.bottomCenter,
                child: AnimatedContainer(
                  duration: _layoutAnimDuration,
                  curve: _layoutAnimCurve,
                  width: contentWidth,
                  height: contentHeight.clamp(0, constraints.maxHeight),
                  child: Stack(
                    clipBehavior: Clip.none,
                    alignment: Alignment.bottomLeft,
                    children: [
                      for (var gi = 0; gi < groups.length; gi++)
                        AnimatedPositioned(
                          key: ValueKey(_groupKey(groups[gi])),
                          duration: _layoutAnimDuration,
                          curve: _layoutAnimCurve,
                          left: gi * hStep,
                          bottom: 0,
                          width: cardWidth,
                          height: cardHeight + (groups[gi].length - 1) * vStep,
                          child: Stack(
                            clipBehavior: Clip.none,
                            alignment: Alignment.bottomCenter,
                            children: [
                              for (var i = 0; i < groups[gi].length; i++)
                                Positioned(
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
                        ),
                    ],
                  ),
                ),
              ),
            ),
          );
        },
      ),
    );
  }

  String _groupKey(List<GameCard> group) {
    return group.map((c) => c.id).join('_');
  }
}
