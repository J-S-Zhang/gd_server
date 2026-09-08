import 'package:flutter/material.dart';
import '../models/card.dart';
import '../utils/hand_layout.dart';
import 'poker_card.dart';

class HandCardsWidget extends StatelessWidget {
  final List<GameCard> cards;
  final int currentLevel;
  final void Function(int cardId) onCardTap;

  static const double _cardWidth = 50;
  static const double _cardHeight = 72;
  static const double _stackOffset = 10;

  const HandCardsWidget({
    super.key,
    required this.cards,
    required this.currentLevel,
    required this.onCardTap,
  });

  @override
  Widget build(BuildContext context) {
    if (cards.isEmpty) {
      return const SizedBox(
        height: 90,
        child: Center(
          child: Text('等待发牌...', style: TextStyle(color: Colors.white54)),
        ),
      );
    }

    final groups = groupHandCardsByRank(cards, currentLevel: currentLevel);
    final maxStackDepth = groups.fold<int>(
      1,
      (max, group) => group.length > max ? group.length : max,
    );
    final rowHeight = _cardHeight + (maxStackDepth - 1) * _stackOffset + 12;

    return SizedBox(
      height: rowHeight,
      child: SingleChildScrollView(
        scrollDirection: Axis.horizontal,
        padding: const EdgeInsets.symmetric(horizontal: 8),
        child: Row(
          crossAxisAlignment: CrossAxisAlignment.end,
          children: groups.map((group) {
            final stackHeight = _cardHeight + (group.length - 1) * _stackOffset;
            return Padding(
              padding: const EdgeInsets.symmetric(horizontal: 2),
              child: SizedBox(
                width: _cardWidth,
                height: stackHeight,
                child: Stack(
                  clipBehavior: Clip.none,
                  alignment: Alignment.bottomCenter,
                  children: [
                    for (var i = 0; i < group.length; i++)
                      Positioned(
                        bottom: i * _stackOffset,
                        child: PokerCardWidget(
                          card: group[i],
                          width: _cardWidth,
                          height: _cardHeight,
                          onTap: () => onCardTap(group[i].id),
                        ),
                      ),
                  ],
                ),
              ),
            );
          }).toList(),
        ),
      ),
    );
  }
}
