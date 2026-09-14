import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'app.dart';
import 'config/server_config_store.dart';
import 'config/ui_config.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await UiConfigBundle.load();
  await ServerConfigStore.init();
  runApp(const ProviderScope(child: GuandanApp()));
}
