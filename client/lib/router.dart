import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';
import 'pages/login/login_page.dart';
import 'pages/lobby/lobby_page.dart';
import 'pages/room/room_page.dart';
import 'pages/game/game_page.dart';
import 'pages/result/result_page.dart';

final appRouter = GoRouter(
  initialLocation: '/login',
  routes: [
    GoRoute(
      path: '/login',
      builder: (context, state) => const LoginPage(),
    ),
    GoRoute(
      path: '/lobby',
      builder: (context, state) => const LobbyPage(),
    ),
    GoRoute(
      path: '/room/:roomId',
      builder: (context, state) {
        final roomId = state.pathParameters['roomId'] ?? '';
        return RoomPage(roomId: roomId);
      },
    ),
    GoRoute(
      path: '/game/:roomId',
      builder: (context, state) {
        final roomId = state.pathParameters['roomId'] ?? '';
        return GamePage(roomId: roomId);
      },
    ),
    GoRoute(
      path: '/result/:roomId',
      builder: (context, state) => const ResultPage(),
    ),
  ],
);
