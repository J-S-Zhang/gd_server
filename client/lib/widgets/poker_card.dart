import 'package:flutter/material.dart';
import '../models/card.dart';

class PokerCardWidget extends StatelessWidget {
  final GameCard card;
  final VoidCallback? onTap;
  final double width;
  final double height;

  const PokerCardWidget({
    super.key,
    required this.card,
    this.onTap,
    this.width = 50,
    this.height = 72,
  });

  @override
  Widget build(BuildContext context) {
    final isRed = card.suit == Suit.heart || card.suit == Suit.diamond;
    final color = isRed ? Colors.red : Colors.black;

    return GestureDetector(
      onTap: onTap,
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 150),
        transform: Matrix4.translationValues(0, card.selected ? -12 : 0, 0),
        width: width,
        height: height,
        decoration: BoxDecoration(
          color: card.selected ? Colors.amber.shade100 : Colors.white,
          borderRadius: BorderRadius.circular(6),
          border: Border.all(
            color: card.selected ? Colors.amber : Colors.grey.shade400,
            width: card.selected ? 2 : 1,
          ),
          boxShadow: [
            BoxShadow(
              color: Colors.black.withValues(alpha: 0.3),
              blurRadius: 4,
              offset: const Offset(1, 2),
            ),
          ],
        ),
        child: Center(
          child: Text(
            card.displayName,
            style: TextStyle(
              color: color,
              fontWeight: FontWeight.bold,
              fontSize: width * 0.28,
            ),
          ),
        ),
      ),
    );
  }
}
