import '../config/server_config.dart';

enum PlayerStatus { online, offline }

class Player {
  final int id;
  final String nickname;
  final String? avatar;
  final String? avatarPreset;
  final int seatIndex;
  final int team;
  final int cardCount;
  final bool hasFinished;
  final int finishRank;
  final bool isReady;
  final bool isBot;
  final PlayerStatus status;

  const Player({
    required this.id,
    required this.nickname,
    this.avatar,
    this.avatarPreset,
    required this.seatIndex,
    required this.team,
    this.cardCount = 0,
    this.hasFinished = false,
    this.finishRank = 0,
    this.isReady = false,
    this.isBot = false,
    this.status = PlayerStatus.online,
  });

  String? get avatarUrl {
    final url = ServerConfig.resolveMediaUrl(avatar);
    return url.isEmpty ? null : url;
  }

  factory Player.fromJson(Map<String, dynamic> json) {
    return Player(
      id: json['id'] as int,
      nickname: json['nickname'] as String? ?? 'Player',
      avatar: json['avatar'] as String?,
      avatarPreset: json['avatar_preset'] as String?,
      seatIndex: json['seat_index'] as int? ?? 0,
      team: json['team'] as int? ?? 0,
      cardCount: json['card_count'] as int? ?? 0,
      hasFinished: json['has_finished'] as bool? ?? false,
      finishRank: json['finish_rank'] as int? ?? 0,
      isReady: json['is_ready'] as bool? ?? false,
      isBot: json['is_bot'] as bool? ?? (json['id'] as int? ?? 0) >= 9000000000,
      status: json['status'] == 'offline'
          ? PlayerStatus.offline
          : PlayerStatus.online,
    );
  }
}
