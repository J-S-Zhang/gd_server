import 'dart:convert';
import 'dart:io';
import 'package:http/http.dart' as http;
import '../utils/constants.dart';
import '../models/user.dart';

class HttpClient {
  String? _token;

  void setToken(String token) => _token = token;

  Map<String, String> get _headers => {
        'Content-Type': 'application/json',
        'Accept': 'application/json',
        'Connection': 'close',
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
    Object? lastError;
    for (var attempt = 0; attempt < 2; attempt++) {
      try {
        final response = await http
            .post(
              Uri.parse(url),
              headers: _headers,
              body: jsonEncode(body),
            )
            .timeout(const Duration(seconds: 15));

        final Map<String, dynamic> data =
            jsonDecode(response.body) as Map<String, dynamic>;
        if (response.statusCode == 200) {
          return User.fromJson(data);
        }

        final message = data['message'] as String? ?? '请求失败';
        throw Exception(message);
      } on SocketException catch (e) {
        lastError = Exception('网络连接失败，请检查网络或稍后重试（${e.message}）');
      } on HttpException catch (e) {
        lastError = Exception('网络请求异常：${e.message}');
      } catch (e) {
        lastError = e;
      }
      if (attempt == 0) {
        await Future<void>.delayed(const Duration(milliseconds: 500));
        continue;
      }
      break;
    }
    if (lastError is Exception) throw lastError!;
    throw Exception('网络连接失败，请稍后重试');
  }
}
