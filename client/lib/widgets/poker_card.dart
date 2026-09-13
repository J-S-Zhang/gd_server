import 'package:flutter/material.dart';
import '../config/ui_scale.dart';
import '../models/card.dart';
import '../utils/card_rules.dart';
import '../utils/card_utils.dart';

class PokerCardWidget extends StatelessWidget {
  final GameCard card;
  final VoidCallback? onTap;
  final double width;
  final double height;
  final int currentLevel;

  const PokerCardWidget({
    super.key,
    required this.card,
    this.onTap,
    this.width = 50,
    this.height = 72,
    this.currentLevel = 2,
  });

  @override
  Widget build(BuildContext context) {
    final ui = context.ui;
    final cardCfg = ui.config.card;
    final isWild = isWildCard(card, currentLevel);
    final isLevel = !isWild && isLevelCard(card, currentLevel);
    final selectionLift = ui.h(ui.config.handCards.selectionLift);
    final radius = ui.r(cardCfg.borderRadius);

    return GestureDetector(
      onTap: onTap,
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 150),
        transform: Matrix4.translationValues(0, card.selected ? -selectionLift : 0, 0),
        width: width,
        height: height,
        decoration: BoxDecoration(
          borderRadius: BorderRadius.circular(radius),
          border: Border.all(
            color: card.selected ? const Color(0xFFFFC107) : Colors.transparent,
            width: card.selected ? ui.r(cardCfg.selectedBorderWidth) : 0,
          ),
          boxShadow: [
            BoxShadow(
              color: Colors.black.withValues(alpha: card.selected ? 0.45 : 0.28),
              blurRadius: ui.r(card.selected ? 8 : 4),
              offset: Offset(0, ui.h(card.selected ? 3 : 2)),
            ),
          ],
        ),
        child: ClipRRect(
          borderRadius: BorderRadius.circular(ui.r(cardCfg.clipRadius)),
          child: Stack(
            fit: StackFit.expand,
            children: [
              Image.asset(
                cardAssetPath(card),
                fit: BoxFit.cover,
                errorBuilder: (_, __, ___) => ColoredBox(
                  color: const Color(0xFFF5F5F5),
                  child: Center(
                    child: Text(
                      card.displayName,
                      style: TextStyle(
                        color: card.suit == Suit.heart || card.suit == Suit.diamond
                            ? const Color(0xFFD32F2F)
                            : const Color(0xFF212121),
                        fontWeight: FontWeight.bold,
                        fontSize: width * cardCfg.labelFontRatio,
                      ),
                    ),
                  ),
                ),
              ),
              if (isWild)
                _CardCornerMark.vertical(
                  text: '逢人配',
                  cardWidth: width,
                  charWidthRatio: cardCfg.cornerMarkCharWidthRatio,
                  fontScale: cardCfg.cornerMarkFontScale,
                  color: const Color(0xFFD32F2F),
                )
              else if (isLevel)
                _CardCornerMark.single(
                  text: '级',
                  cardWidth: width,
                  charWidthRatio: cardCfg.cornerMarkCharWidthRatio,
                  fontScale: cardCfg.cornerMarkFontScale,
                  color: const Color(0xFFFFC107),
                ),
            ],
          ),
        ),
      ),
    );
  }
}

class _CardCornerMark extends StatelessWidget {
  final String text;
  final double cardWidth;
  final double charWidthRatio;
  final double fontScale;
  final Color color;
  final bool vertical;

  const _CardCornerMark._({
    required this.text,
    required this.cardWidth,
    required this.charWidthRatio,
    required this.fontScale,
    required this.color,
    required this.vertical,
  });

  factory _CardCornerMark.single({
    required String text,
    required double cardWidth,
    required double charWidthRatio,
    required double fontScale,
    required Color color,
  }) {
    return _CardCornerMark._(
      text: text,
      cardWidth: cardWidth,
      charWidthRatio: charWidthRatio,
      fontScale: fontScale,
      color: color,
      vertical: false,
    );
  }

  factory _CardCornerMark.vertical({
    required String text,
    required double cardWidth,
    required double charWidthRatio,
    required double fontScale,
    required Color color,
  }) {
    return _CardCornerMark._(
      text: text,
      cardWidth: cardWidth,
      charWidthRatio: charWidthRatio,
      fontScale: fontScale,
      color: color,
      vertical: true,
    );
  }

  double get _charWidth => cardWidth * charWidthRatio;

  TextStyle _charStyle() {
    return TextStyle(
      color: color,
      fontSize: _charWidth * fontScale,
      fontWeight: FontWeight.bold,
      height: 1,
      shadows: const [
        Shadow(color: Colors.black54, blurRadius: 2, offset: Offset(0, 1)),
        Shadow(color: Colors.white70, blurRadius: 1, offset: Offset(0, 0)),
      ],
    );
  }

  Widget _charCell(String char) {
    return SizedBox(
      width: _charWidth,
      height: _charWidth,
      child: Center(
        child: Text(char, style: _charStyle(), textAlign: TextAlign.center),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Positioned(
      left: 0,
      bottom: 0,
      child: vertical
          ? Column(
              mainAxisSize: MainAxisSize.min,
              crossAxisAlignment: CrossAxisAlignment.start,
              children: text.split('').map(_charCell).toList(),
            )
          : _charCell(text),
    );
  }
}
