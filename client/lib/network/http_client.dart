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

  Future<User> login(String nickname, String password) async {
    return _authRequest(
      '${Constants.apiBaseUrl}/api/login',
      {'nickname': nickname.trim(), 'password': password},
    );
  }

  Future<User> register(String nickname, String password) async {
    return _authRequest(
      '${Constants.apiBaseUrl}/api/register',
      {'nickname': nickname.trim(), 'password': password},
    );
  }

  Future<User> _authRequest(String url, Map<String, dynamic> body) async {
    final response = await http.post(
      Uri.parse(url),
      headers: _headers,
      body: jsonEncode(body),
    );

    final Map<String, dynamic> data = jsonDecode(response.body) as Map<String, dynamic>;
    if (response.statusCode == 200) {
      return User.fromJson(data);
    }

    final message = data['message'] as String? ?? '请求失败';
    throw Exception(message);
  }
}
