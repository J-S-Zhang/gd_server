import '../config/server_config.dart';

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
  final String? avatarPreset;
  final String token;
  final UserStats stats;

  const User({
    required this.id,
    required this.username,
    required this.nickname,
    this.avatar,
    this.avatarPreset,
    required this.token,
    this.stats = const UserStats(),
  });

  factory User.fromJson(Map<String, dynamic> json) {
    return User(
      id: json['id'] as int,
      username: json['username'] as String? ?? json['nickname'] as String,
      nickname: json['nickname'] as String,
      avatar: json['avatar'] as String?,
      avatarPreset: json['avatar_preset'] as String?,
      token: json['token'] as String? ?? '',
      stats: UserStats.fromJson(json['stats'] as Map<String, dynamic>?),
    );
  }

  User copyWith({
    int? id,
    String? username,
    String? nickname,
    String? avatar,
    String? avatarPreset,
    String? token,
    UserStats? stats,
  }) {
    return User(
      id: id ?? this.id,
      username: username ?? this.username,
      nickname: nickname ?? this.nickname,
      avatar: avatar ?? this.avatar,
      avatarPreset: avatarPreset ?? this.avatarPreset,
      token: token ?? this.token,
      stats: stats ?? this.stats,
    );
  }

  String? get avatarUrl {
    final url = ServerConfig.resolveMediaUrl(avatar);
    return url.isEmpty ? null : url;
  }
}
