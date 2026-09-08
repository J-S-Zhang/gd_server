import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import '../../controller/auth_controller.dart';

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

  InputDecoration _fieldDecoration(String label, IconData icon) {
    return InputDecoration(
      labelText: label,
      prefixIcon: Icon(icon),
      filled: true,
      fillColor: Colors.white,
      border: const OutlineInputBorder(),
    );
  }

  @override
  Widget build(BuildContext context) {
    final isRegister = _tabController.index == 1;

    return Scaffold(
      body: Container(
        decoration: const BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [Color(0xFF1B5E20), Color(0xFF0D3311)],
          ),
        ),
        child: SafeArea(
          child: Center(
            child: SingleChildScrollView(
              padding: const EdgeInsets.all(32),
              child: ConstrainedBox(
                constraints: const BoxConstraints(maxWidth: 420),
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    const Icon(Icons.style, size: 80, color: Colors.amber),
                    const SizedBox(height: 16),
                    Text(
                      '六人掼蛋',
                      style: Theme.of(context).textTheme.headlineLarge?.copyWith(
                            color: Colors.white,
                            fontWeight: FontWeight.bold,
                          ),
                    ),
                    const SizedBox(height: 32),
                    Container(
                      decoration: BoxDecoration(
                        color: Colors.white.withValues(alpha: 0.12),
                        borderRadius: BorderRadius.circular(12),
                      ),
                      child: TabBar(
                        controller: _tabController,
                        indicatorColor: Colors.amber,
                        labelColor: Colors.white,
                        unselectedLabelColor: Colors.white70,
                        onTap: (_) => setState(() {}),
                        tabs: const [
                          Tab(text: '登录'),
                          Tab(text: '注册'),
                        ],
                      ),
                    ),
                    const SizedBox(height: 24),
                    TextField(
                      controller: _nicknameController,
                      decoration: _fieldDecoration('昵称', Icons.person),
                    ),
                    const SizedBox(height: 16),
                    TextField(
                      controller: _passwordController,
                      obscureText: true,
                      decoration: _fieldDecoration('密码', Icons.lock),
                    ),
                    if (isRegister) ...[
                      const SizedBox(height: 16),
                      TextField(
                        controller: _confirmPasswordController,
                        obscureText: true,
                        decoration: _fieldDecoration('确认密码', Icons.lock_outline),
                      ),
                    ],
                    const SizedBox(height: 32),
                    SizedBox(
                      width: double.infinity,
                      height: 48,
                      child: ElevatedButton(
                        onPressed: _loading
                            ? null
                            : (isRegister ? _submitRegister : _submitLogin),
                        style: ElevatedButton.styleFrom(
                          backgroundColor: Colors.amber,
                          foregroundColor: Colors.black,
                        ),
                        child: _loading
                            ? const SizedBox(
                                width: 24,
                                height: 24,
                                child: CircularProgressIndicator(strokeWidth: 2),
                              )
                            : Text(
                                isRegister ? '注册' : '登录',
                                style: const TextStyle(fontSize: 18),
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
