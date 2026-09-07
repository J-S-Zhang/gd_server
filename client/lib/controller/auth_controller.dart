import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../models/user.dart';
import '../network/http_client.dart';

final httpClientProvider = Provider((ref) => HttpClient());

final userProvider = StateProvider<User?>((ref) => null);

final authControllerProvider = Provider((ref) {
  return AuthController(ref);
});

class AuthController {
  final Ref _ref;
  AuthController(this._ref);

  Future<void> login(String username, String password) async {
    final client = _ref.read(httpClientProvider);
    final user = await client.login(username, password);
    client.setToken(user.token);
    _ref.read(userProvider.notifier).state = user;
  }

  void logout() {
    _ref.read(userProvider.notifier).state = null;
  }
}
