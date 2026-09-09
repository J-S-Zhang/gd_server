import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import '../../config/ui_scale.dart';
import '../../controller/auth_controller.dart';
import '../../theme/game_theme.dart';

class LoginPage extends ConsumerStatefulWidget {
  const LoginPage({super.key});

  @override
  ConsumerState<LoginPage> createState() => _LoginPageState();
}

class _LoginPageState extends ConsumerState<LoginPage>
    with SingleTickerProviderStateMixin {
  late final TabController _tabController;
  final _nicknameController = TextEditingController();
  final _passwordController = TextEditingController();
  final _confirmPasswordController = TextEditingController();
  bool _loading = false;

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 2, vsync: this);
  }

  Future<void> _submitLogin() async {
    final nickname = _nicknameController.text.trim();
    final password = _passwordController.text;
    if (nickname.isEmpty) {
      _showError('请输入昵称');
      return;
    }
    if (password.isEmpty) {
      _showError('请输入密码');
      return;
    }

    setState(() => _loading = true);
    try {
      await ref.read(authControllerProvider).login(nickname, password);
      if (mounted) context.go('/lobby');
    } catch (e) {
      _showError('$e'.replaceFirst('Exception: ', ''));
    } finally {
      if (mounted) setState(() => _loading = false);
    }
  }

  Future<void> _submitRegister() async {
    final nickname = _nicknameController.text.trim();
    final password = _passwordController.text;
    final confirm = _confirmPasswordController.text;
    if (nickname.isEmpty) {
      _showError('请输入昵称');
      return;
    }
    if (nickname.length < 2 || nickname.length > 16) {
      _showError('昵称长度需在 2-16 个字符之间');
      return;
    }
    if (password.length < 6) {
      _showError('密码长度至少 6 位');
      return;
    }
    if (password != confirm) {
      _showError('两次输入的密码不一致');
      return;
    }

    setState(() => _loading = true);
    try {
      await ref.read(authControllerProvider).register(nickname, password);
      if (mounted) context.go('/lobby');
    } catch (e) {
      _showError('$e'.replaceFirst('Exception: ', ''));
    } finally {
      if (mounted) setState(() => _loading = false);
    }
  }

  void _showError(String message) {
    if (!mounted) return;
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text(message)),
    );
  }

  InputDecoration _fieldDecoration(UiScale ui, String label, IconData icon) {
    return InputDecoration(
      labelText: label,
      labelStyle: TextStyle(color: Colors.white70, fontSize: ui.sp(ui.config.font.md2)),
      prefixIcon: Icon(icon, color: Colors.white70, size: ui.sp(ui.config.font.xl)),
      filled: true,
      fillColor: Colors.white.withValues(alpha: 0.12),
      enabledBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(ui.r(ui.config.radius.lg)),
        borderSide: BorderSide(color: Colors.white.withValues(alpha: 0.2)),
      ),
      focusedBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(ui.r(ui.config.radius.lg)),
        borderSide: BorderSide(color: GameTheme.accentGold, width: ui.r(2)),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final ui = context.ui;
    final layout = ui.config.layout;
    final isRegister = _tabController.index == 1;

    return Scaffold(
      body: Container(
        decoration: const BoxDecoration(gradient: GameTheme.pageGradient),
        child: SafeArea(
          child: Center(
            child: SingleChildScrollView(
              padding: ui.edgeInsetsAll(ui.config.spacing.page),
              child: ConstrainedBox(
                constraints: BoxConstraints(maxWidth: ui.w(layout.loginMaxWidth)),
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Container(
                      padding: ui.edgeInsetsAll(ui.config.spacing.xl),
                      decoration: GameTheme.panelDecoration(ui),
                      child: Icon(
                        Icons.style,
                        size: ui.sp(layout.loginIconSize),
                        color: GameTheme.accentGold,
                      ),
                    ),
                    SizedBox(height: ui.h(ui.config.spacing.xl)),
                    Text(
                      '六人掼蛋',
                      style: TextStyle(
                        color: Colors.white,
                        fontSize: ui.sp(ui.config.font.title),
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    SizedBox(height: ui.h(ui.config.spacing.md)),
                    Text(
                      '经典棋牌 · 六人实时对战',
                      style: TextStyle(
                        color: GameTheme.textSecondary,
                        fontSize: ui.sp(ui.config.font.md2),
                      ),
                    ),
                    SizedBox(height: ui.h(ui.config.spacing.page)),
                    Container(
                      decoration: GameTheme.panelDecoration(ui),
                      child: TabBar(
                        controller: _tabController,
                        indicatorColor: GameTheme.accentGold,
                        labelColor: Colors.white,
                        unselectedLabelColor: Colors.white54,
                        labelStyle: TextStyle(fontSize: ui.sp(ui.config.font.md2)),
                        onTap: (_) => setState(() {}),
                        tabs: const [
                          Tab(text: '登录'),
                          Tab(text: '注册'),
                        ],
                      ),
                    ),
                    SizedBox(height: ui.h(ui.config.spacing.xxl)),
                    TextField(
                      controller: _nicknameController,
                      style: TextStyle(color: Colors.white, fontSize: ui.sp(ui.config.font.md2)),
                      decoration: _fieldDecoration(ui, '昵称', Icons.person),
                    ),
                    SizedBox(height: ui.h(ui.config.spacing.xl)),
                    TextField(
                      controller: _passwordController,
                      obscureText: true,
                      style: TextStyle(color: Colors.white, fontSize: ui.sp(ui.config.font.md2)),
                      decoration: _fieldDecoration(ui, '密码', Icons.lock),
                    ),
                    if (isRegister) ...[
                      SizedBox(height: ui.h(ui.config.spacing.xl)),
                      TextField(
                        controller: _confirmPasswordController,
                        obscureText: true,
                        style: TextStyle(color: Colors.white, fontSize: ui.sp(ui.config.font.md2)),
                        decoration: _fieldDecoration(ui, '确认密码', Icons.lock_outline),
                      ),
                    ],
                    SizedBox(height: ui.h(ui.config.spacing.page)),
                    SizedBox(
                      width: double.infinity,
                      height: ui.h(layout.loginButtonHeight),
                      child: ElevatedButton(
                        onPressed: _loading
                            ? null
                            : (isRegister ? _submitRegister : _submitLogin),
                        style: GameTheme.playButtonStyle(ui).copyWith(
                          minimumSize: WidgetStateProperty.all(
                            Size(double.infinity, ui.h(layout.loginButtonHeight)),
                          ),
                        ),
                        child: _loading
                            ? SizedBox(
                                width: ui.w(ui.config.spacing.xxl),
                                height: ui.h(ui.config.spacing.xxl),
                                child: const CircularProgressIndicator(strokeWidth: 2),
                              )
                            : Text(
                                isRegister ? '注册' : '登录',
                                style: TextStyle(fontSize: ui.sp(ui.config.font.xl)),
                              ),
                      ),
                    ),
                  ],
                ),
              ),
            ),
          ),
        ),
      ),
    );
  }

  @override
  void dispose() {
    _tabController.dispose();
    _nicknameController.dispose();
    _passwordController.dispose();
    _confirmPasswordController.dispose();
    super.dispose();
  }
}
