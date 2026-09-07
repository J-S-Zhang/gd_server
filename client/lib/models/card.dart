enum Suit { spade, heart, club, diamond, joker }

enum Rank {
  r2, r3, r4, r5, r6, r7, r8, r9, r10, j, q, k, a, smallJoker, bigJoker
}

class GameCard {
  final int id;
  final Suit suit;
  final Rank rank;
  bool selected;

  GameCard({
    required this.id,
    required this.suit,
    required this.rank,
    this.selected = false,
  });

  factory GameCard.fromJson(Map<String, dynamic> json) {
    return GameCard(
      id: json['id'] as int,
      suit: Suit.values[json['suit'] as int],
      rank: Rank.values[json['rank'] as int],
    );
  }

  String get displayName {
    if (suit == Suit.joker) {
      return rank == Rank.smallJoker ? '小王' : '大王';
    }
    const rankNames = ['', '', '2', '3', '4', '5', '6', '7', '8', '9', '10', 'J', 'Q', 'K', 'A'];
    const suitNames = ['♠', '♥', '♣', '♦'];
    return '${rankNames[rank.index + 2]}${suitNames[suit.index]}';
  }
}
