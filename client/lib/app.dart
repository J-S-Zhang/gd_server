import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:google_fonts/google_fonts.dart';
import 'config/ui_scale.dart';
import 'controller/room_controller.dart';
import 'router.dart';

class GuandanApp extends ConsumerStatefulWidget {
  const GuandanApp({super.key});

  @override
  ConsumerState<GuandanApp> createState() => _GuandanAppState();
}

class _GuandanAppState extends ConsumerState<GuandanApp> {
  @override
  void initState() {
    super.initState();
    _lockLandscape();
  }

  Future<void> _lockLandscape() async {
    await SystemChrome.setPreferredOrientations([
      DeviceOrientation.landscapeLeft,
      DeviceOrientation.landscapeRight,
    ]);
    await SystemChrome.setEnabledSystemUIMode(SystemUiMode.immersiveSticky);
  }

  @override
  void dispose() {
    SystemChrome.setPreferredOrientations(DeviceOrientation.values);
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    ref.listen<String?>(pendingNavigationProvider, (prev, next) {
      if (next != null) {
        ref.read(pendingNavigationProvider.notifier).state = null;
        appRouter.go(next);
      }
    });

    ref.listen<String?>(wsErrorProvider, (prev, next) {
      if (next != null) {
        final messenger = scaffoldMessengerKey.currentState;
        messenger?.showSnackBar(SnackBar(content: Text(next)));
        ref.read(wsErrorProvider.notifier).state = null;
      }
    });

    final baseTheme = ThemeData(
      colorScheme: ColorScheme.fromSeed(
        seedColor: const Color(0xFF1565A8),
        brightness: Brightness.dark,
      ),
      useMaterial3: true,
    );

    return UiScopeBuilder(
      child: MaterialApp.router(
        title: '六人掼蛋',
        locale: const Locale('zh', 'CN'),
        scaffoldMessengerKey: scaffoldMessengerKey,
        theme: baseTheme.copyWith(
          textTheme: GoogleFonts.notoSansScTextTheme(baseTheme.textTheme),
          primaryTextTheme: GoogleFonts.notoSansScTextTheme(baseTheme.primaryTextTheme),
        ),
        routerConfig: appRouter,
      ),
    );
  }
}

/// 全局 SnackBar，任意页面可显示错误提示
final scaffoldMessengerKey = GlobalKey<ScaffoldMessengerState>();
