import 'package:flutter/material.dart';
import 'router.dart';

class GuandanApp extends StatelessWidget {
  const GuandanApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp.router(
      title: '六人掼蛋',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(
          seedColor: const Color(0xFF1B5E20),
          brightness: Brightness.dark,
        ),
        useMaterial3: true,
      ),
      routerConfig: appRouter,
    );
  }
}
