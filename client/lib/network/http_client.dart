import 'dart:convert';
import 'package:http/http.dart' as http;
import '../utils/constants.dart';
import '../models/user.dart';

class HttpClient {
  String? _token;

  void setToken(String token) => _token = token;

  Map<String, String> get _headers => {
        'Content-Type': 'application/json',
        if (_token != null) 'Authorization': 'Bearer $_token',
      };

  Future<User> login(String username, String password) async {
    // MVP: 本地模拟登录，正式版对接 HTTPS API
    await Future.delayed(const Duration(milliseconds: 500));
    if (username.isEmpty) throw Exception('用户名不能为空');
    return User(
      id: 10001,
      username: username,
      nickname: username,
      token: 'mock_token_${DateTime.now().millisecondsSinceEpoch}',
    );
  }

  Future<User> register(String username, String password, String nickname) async {
    final response = await http.post(
      Uri.parse('${Constants.apiBaseUrl}/register'),
      headers: _headers,
      body: jsonEncode({
        'username': username,
        'password': password,
        'nickname': nickname,
      }),
    );
    if (response.statusCode != 200) {
      throw Exception('注册失败');
    }
    return User.fromJson(jsonDecode(response.body));
  }
}
