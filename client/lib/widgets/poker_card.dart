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
    final color = isRed ? const Color(0xFFD32F2F) : const Color(0xFF212121);

    return GestureDetector(
      onTap: onTap,
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 150),
        transform: Matrix4.translationValues(0, card.selected ? -14 : 0, 0),
        width: width,
        height: height,
        decoration: BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topLeft,
            end: Alignment.bottomRight,
            colors: card.selected
                ? [const Color(0xFFFFF8E1), const Color(0xFFFFECB3)]
                : [Colors.white, const Color(0xFFF5F5F5)],
          ),
          borderRadius: BorderRadius.circular(6),
          border: Border.all(
            color: card.selected ? const Color(0xFFFFC107) : const Color(0xFFBDBDBD),
            width: card.selected ? 2.5 : 1,
          ),
          boxShadow: [
            BoxShadow(
              color: Colors.black.withValues(alpha: card.selected ? 0.45 : 0.28),
              blurRadius: card.selected ? 8 : 4,
              offset: Offset(0, card.selected ? 3 : 2),
            ),
          ],
        ),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Text(
              card.displayName,
              style: TextStyle(
                color: color,
                fontWeight: FontWeight.bold,
                fontSize: width * 0.28,
              ),
            ),
          ],
        ),
      ),
    );
  }
}
