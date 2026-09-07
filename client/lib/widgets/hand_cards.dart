import 'package:flutter/material.dart';
import '../models/card.dart';
import 'poker_card.dart';

class HandCardsWidget extends StatelessWidget {
  final List<GameCard> cards;
  final void Function(int cardId) onCardTap;

  const HandCardsWidget({
    super.key,
    required this.cards,
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

    return SizedBox(
      height: 100,
      child: SingleChildScrollView(
        scrollDirection: Axis.horizontal,
        padding: const EdgeInsets.symmetric(horizontal: 8),
        child: Row(
          children: cards.map((card) {
            return Padding(
              padding: const EdgeInsets.symmetric(horizontal: 2),
              child: PokerCardWidget(
                card: card,
                onTap: () => onCardTap(card.id),
              ),
            );
          }).toList(),
        ),
      ),
    );
  }
}
