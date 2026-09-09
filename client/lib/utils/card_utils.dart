import '../models/card.dart';

/// 根据服务端 CardId 还原牌面（3副牌，与服务端 createFullDeck 一致）
GameCard cardFromId(int id) {
  int cursor = 0;
  for (int deck = 0; deck < 3; deck++) {
    for (int s = 0; s < 4; s++) {
      for (int r = 2; r <= 14; r++) {
        if (cursor == id) {
          return GameCard(
            id: id,
            suit: Suit.values[s],
            rank: Rank.values[r - 2],
          );
        }
        cursor++;
      }
    }
    // 小王
    if (cursor == id) {
      return GameCard(id: id, suit: Suit.joker, rank: Rank.smallJoker);
    }
    cursor++;
    // 大王
    if (cursor == id) {
      return GameCard(id: id, suit: Suit.joker, rank: Rank.bigJoker);
    }
    cursor++;
  }
  return GameCard(id: id, suit: Suit.spade, rank: Rank.r2);
}

List<GameCard> cardsFromIds(List<int> ids) {
  return ids.map(cardFromId).toList();
}

/// 牌面 PNG 资源路径（与 assets/images/cards/ 命名一致）
String cardAssetPath(GameCard card) {
  if (card.suit == Suit.joker) {
    return card.rank == Rank.smallJoker
        ? 'assets/images/cards/Little_Joker.png'
        : 'assets/images/cards/Big_Joker.png';
  }
  const suitNames = ['Spade', 'Heart', 'Club', 'Diamond'];
  const rankNames = ['2', '3', '4', '5', '6', '7', '8', '9', '10', 'J', 'Q', 'K', 'A'];
  return 'assets/images/cards/${suitNames[card.suit.index]}_${rankNames[card.rank.index]}.png';
}
