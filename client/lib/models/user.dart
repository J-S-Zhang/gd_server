class UserStats {
  final int totalGames;
  final int wins;
  final int losses;

  const UserStats({
    this.totalGames = 0,
    this.wins = 0,
    this.losses = 0,
  });

  factory UserStats.fromJson(Map<String, dynamic>? json) {
    if (json == null) return const UserStats();
    return UserStats(
      totalGames: json['total_games'] as int? ?? 0,
      wins: json['wins'] as int? ?? 0,
      losses: json['losses'] as int? ?? 0,
    );
  }
}

class User {
  final int id;
  final String username;
  final String nickname;
  final String? avatar;
  final String token;
  final UserStats stats;

  const User({
    required this.id,
    required this.username,
    required this.nickname,
    this.avatar,
    required this.token,
    this.stats = const UserStats(),
  });

  factory User.fromJson(Map<String, dynamic> json) {
    return User(
      id: json['id'] as int,
      username: json['username'] as String? ?? json['nickname'] as String,
      nickname: json['nickname'] as String,
      avatar: json['avatar'] as String?,
      token: json['token'] as String,
      stats: UserStats.fromJson(json['stats'] as Map<String, dynamic>?),
    );
  }
}
