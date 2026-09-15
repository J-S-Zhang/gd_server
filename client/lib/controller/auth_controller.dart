import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../config/avatar_active_preset_store.dart';
import '../config/avatar_presets.dart';
import '../models/user.dart';
import '../network/http_client.dart';

final httpClientProvider = Provider((ref) => HttpClient());

final userProvider = StateProvider<User?>((ref) => null);

/// 头像 URL 不变时用于打破 [Image.network] 缓存（每次上传后递增）。
final avatarCacheRevisionProvider = StateProvider<int>((ref) => 0);

/// 当前使用的内置头像预设 id；有值时优先显示本地 assets 图。
final activeAvatarPresetIdProvider = StateProvider<String?>((ref) => null);

final authControllerProvider = Provider((ref) {
  return AuthController(ref);
});

class AuthController {
  final Ref _ref;
  AuthController(this._ref);

  Future<void> syncActiveAvatarPreset() async {
    final user = _ref.read(userProvider);
    String? presetId;
    final serverPreset = user?.avatarPreset;
    if (serverPreset != null &&
        serverPreset.isNotEmpty &&
        avatarPresetById(serverPreset) != null) {
      presetId = serverPreset;
      await AvatarActivePresetStore.save(serverPreset);
    } else {
      final storedId = await AvatarActivePresetStore.load();
      presetId =
          storedId != null && avatarPresetById(storedId) != null ? storedId : null;
    }
    _ref.read(activeAvatarPresetIdProvider.notifier).state = presetId;
  }

  Future<void> login(String nickname, String password) async {
    final client = _ref.read(httpClientProvider);
    final user = await client.login(nickname, password);
    client.setToken(user.token);
    _ref.read(userProvider.notifier).state = user;
    await syncActiveAvatarPreset();
  }

  Future<void> register(String nickname, String password) async {
    final client = _ref.read(httpClientProvider);
    final user = await client.register(nickname, password);
    client.setToken(user.token);
    _ref.read(userProvider.notifier).state = user;
    await syncActiveAvatarPreset();
  }

  Future<User> uploadAvatar(List<int> bytes, String format) async {
    final client = _ref.read(httpClientProvider);
    final updated = await client.uploadAvatar(bytes, format);
    final current = _ref.read(userProvider);
    final merged = updated.copyWith(
      token: updated.token.isNotEmpty ? updated.token : current?.token ?? '',
      avatarPreset: updated.avatarPreset ?? current?.avatarPreset,
    );
    client.setToken(merged.token);
    _ref.read(userProvider.notifier).state = merged;
    _ref.read(avatarCacheRevisionProvider.notifier).state =
        DateTime.now().millisecondsSinceEpoch;
    return merged;
  }

  Future<User> saveAvatarPreset(String presetId) async {
    final client = _ref.read(httpClientProvider);
    final updated = await client.updateAvatarPreset(presetId);
    final current = _ref.read(userProvider);
    final merged = updated.copyWith(
      token: updated.token.isNotEmpty ? updated.token : current?.token ?? '',
      avatar: updated.avatar ?? current?.avatar,
    );
    client.setToken(merged.token);
    _ref.read(userProvider.notifier).state = merged;
    await AvatarActivePresetStore.save(presetId);
    _ref.read(activeAvatarPresetIdProvider.notifier).state = presetId;
    return merged;
  }

  void logout() {
    _ref.read(userProvider.notifier).state = null;
    _ref.read(activeAvatarPresetIdProvider.notifier).state = null;
    _ref.read(avatarCacheRevisionProvider.notifier).state = 0;
  }
}
