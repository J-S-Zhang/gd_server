class User {
  final int id;
  final String username;
  final String nickname;
  final String? avatar;
  final String token;

  const User({
    required this.id,
    required this.username,
    required this.nickname,
    this.avatar,
    required this.token,
  });

  factory User.fromJson(Map<String, dynamic> json) {
    return User(
      id: json['id'] as int,
      username: json['username'] as String,
      nickname: json['nickname'] as String,
      avatar: json['avatar'] as String?,
      token: json['token'] as String,
    );
  }
}
