import 'dart:math' as math;

import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import '../../config/ui_scale.dart';
import '../../controller/auth_controller.dart';
import '../../services/app_update_service.dart';
import '../../theme/game_theme.dart';
import '../../widgets/app_update_dialog.dart';

/// 登录页按可用区域比例分配尺寸，保证一屏完整显示、无需滚动。
class _LoginLayoutMetrics {
  const _LoginLayoutMetrics({
    required this.formWidth,
    required this.iconSize,
    required this.iconPadding,
    required this.titleSize,
    required this.subtitleSize,
    required this.tabFontSize,
    required this.fieldFontSize,
    required this.fieldIconSize,
    required this.buttonFontSize,
    required this.buttonHeight,
    required this.radius,
    required this.flexHeader,
    required this.flexTab,
    required this.flexField,
    required this.flexButton,
  });

  final double formWidth;
  final double iconSize;
  final double iconPadding;
  final double titleSize;
  final double subtitleSize;
  final double tabFontSize;
  final double fieldFontSize;
  final double fieldIconSize;
  final double buttonFontSize;
  final double buttonHeight;
  final double radius;
  final int flexHeader;
  final int flexTab;
  final int flexField;
  final int flexButton;

  factory _LoginLayoutMetrics.fromSize(Size size, bool isRegister) {
    final w = size.width;
    final h = size.height;
    final shortSide = size.shortestSide;
    final scale = math.min(w / 844, h / 390);

    const flexHeader = 24;
    const flexTab = 8;
    const flexField = 10;
    const flexButton = 10;
    final fitScale = scale.clamp(0.75, 1.25);

    final formWidthFactor = shortSide < 600
        ? 0.86
        : shortSide < 900
            ? 0.52
            : 0.42;

    return _LoginLayoutMetrics(
      formWidth: w * formWidthFactor,
      iconSize: h * 0.11 * fitScale,
      iconPadding: h * 0.018 * fitScale,
      titleSize: (h * 0.062 * fitScale).clamp(18.0, 40.0),
      subtitleSize: (h * 0.032 * fitScale).clamp(11.0, 18.0),
      tabFontSize: (h * 0.034 * fitScale).clamp(12.0, 18.0),
      fieldFontSize: (h * 0.034 * fitScale).clamp(12.0, 18.0),
      fieldIconSize: (h * 0.042 * fitScale).clamp(14.0, 22.0),
      buttonFontSize: (h * 0.038 * fitScale).clamp(13.0, 20.0),
      buttonHeight: h * 0.095 * fitScale,
      radius: 12 * fitScale,
      flexHeader: flexHeader,
      flexTab: flexTab,
      flexField: flexField,
      flexButton: flexButton,
    );
  }
}

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
  final _updateService = AppUpdateService();

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 2, vsync: this);
    WidgetsBinding.instance.addPostFrameCallback((_) => _checkAppUpdate());
  }

  Future<void> _checkAppUpdate() async {
    final result = await _updateService.checkForUpdate();
    if (!mounted || !result.hasUpdate || result.remote == null) return;

    await showAppUpdateDialog(
      context: context,
      versionInfo: result.remote!,
      updateService: _updateService,
    );
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

  InputDecoration _fieldDecoration(
    _LoginLayoutMetrics metrics,
    String label,
    IconData icon,
  ) {
    return InputDecoration(
      labelText: label,
      labelStyle: TextStyle(color: Colors.white70, fontSize: metrics.fieldFontSize),
      prefixIcon: Icon(icon, color: Colors.white70, size: metrics.fieldIconSize),
      filled: true,
      fillColor: Colors.white.withValues(alpha: 0.12),
      isDense: true,
      contentPadding: EdgeInsets.symmetric(
        horizontal: metrics.fieldFontSize,
        vertical: metrics.fieldFontSize * 0.55,
      ),
      enabledBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(metrics.radius),
        borderSide: BorderSide(color: Colors.white.withValues(alpha: 0.2)),
      ),
      focusedBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(metrics.radius),
        borderSide: BorderSide(color: GameTheme.accentGold, width: metrics.radius * 0.15),
      ),
    );
  }

  Widget _buildField({
    required _LoginLayoutMetrics metrics,
    required TextEditingController controller,
    required String label,
    required IconData icon,
    bool obscure = false,
  }) {
    return Expanded(
      flex: metrics.flexField,
      child: LayoutBuilder(
        builder: (context, fieldConstraints) {
          return Align(
            alignment: Alignment.center,
            child: SizedBox(
              height: fieldConstraints.maxHeight * 0.88,
              child: TextField(
                controller: controller,
                obscureText: obscure,
                style: TextStyle(
                  color: Colors.white,
                  fontSize: metrics.fieldFontSize,
                ),
                decoration: _fieldDecoration(metrics, label, icon),
              ),
            ),
          );
        },
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final ui = context.ui;
    final isRegister = _tabController.index == 1;

    return Scaffold(
      resizeToAvoidBottomInset: false,
      body: Container(
        decoration: const BoxDecoration(gradient: GameTheme.pageGradient),
        child: SafeArea(
          child: LayoutBuilder(
            builder: (context, constraints) {
              final metrics = _LoginLayoutMetrics.fromSize(
                constraints.biggest,
                isRegister,
              );

              return Center(
                child: SizedBox(
                  width: metrics.formWidth,
                  height: constraints.maxHeight,
                  child: Column(
                    children: [
                      Expanded(
                        flex: metrics.flexHeader,
                        child: FittedBox(
                          fit: BoxFit.scaleDown,
                          child: Column(
                            mainAxisSize: MainAxisSize.min,
                            children: [
                              Container(
                                padding: EdgeInsets.all(metrics.iconPadding),
                                decoration: GameTheme.panelDecoration(
                                  ui,
                                  radius: metrics.radius,
                                ),
                                child: Icon(
                                  Icons.style,
                                  size: metrics.iconSize,
                                  color: GameTheme.accentGold,
                                ),
                              ),
                              SizedBox(height: metrics.subtitleSize * 0.6),
                              Text(
                                '六人掼蛋',
                                style: TextStyle(
                                  color: Colors.white,
                                  fontSize: metrics.titleSize,
                                  fontWeight: FontWeight.bold,
                                ),
                              ),
                              SizedBox(height: metrics.subtitleSize * 0.35),
                              Text(
                                '经典棋牌 · 六人实时对战',
                                style: TextStyle(
                                  color: GameTheme.textSecondary,
                                  fontSize: metrics.subtitleSize,
                                ),
                              ),
                            ],
                          ),
                        ),
                      ),
                      Expanded(
                        flex: metrics.flexTab,
                        child: LayoutBuilder(
                          builder: (context, tabConstraints) {
                            return Center(
                              child: SizedBox(
                                height: tabConstraints.maxHeight * 0.9,
                                child: Container(
                                  decoration: GameTheme.panelDecoration(
                                    ui,
                                    radius: metrics.radius,
                                  ),
                                  child: TabBar(
                                    controller: _tabController,
                                    indicatorColor: GameTheme.accentGold,
                                    labelColor: Colors.white,
                                    unselectedLabelColor: Colors.white54,
                                    labelStyle:
                                        TextStyle(fontSize: metrics.tabFontSize),
                                    unselectedLabelStyle:
                                        TextStyle(fontSize: metrics.tabFontSize),
                                    onTap: (_) => setState(() {}),
                                    tabs: const [
                                      Tab(text: '登录'),
                                      Tab(text: '注册'),
                                    ],
                                  ),
                                ),
                              ),
                            );
                          },
                        ),
                      ),
                      _buildField(
                        metrics: metrics,
                        controller: _nicknameController,
                        label: '昵称',
                        icon: Icons.person,
                      ),
                      _buildField(
                        metrics: metrics,
                        controller: _passwordController,
                        label: '密码',
                        icon: Icons.lock,
                        obscure: true,
                      ),
                      if (isRegister)
                        _buildField(
                          metrics: metrics,
                          controller: _confirmPasswordController,
                          label: '确认密码',
                          icon: Icons.lock_outline,
                          obscure: true,
                        ),
                      Expanded(
                        flex: metrics.flexButton,
                        child: Padding(
                          padding: EdgeInsets.symmetric(
                            vertical: metrics.fieldFontSize * 0.15,
                          ),
                          child: SizedBox(
                            width: double.infinity,
                            height: metrics.buttonHeight,
                            child: ElevatedButton(
                              onPressed: _loading
                                  ? null
                                  : (isRegister ? _submitRegister : _submitLogin),
                              style: GameTheme.playButtonStyle(ui).copyWith(
                                minimumSize: WidgetStateProperty.all(
                                  Size(double.infinity, metrics.buttonHeight),
                                ),
                                padding: WidgetStateProperty.all(
                                  EdgeInsets.symmetric(
                                    vertical: metrics.buttonFontSize * 0.3,
                                  ),
                                ),
                              ),
                              child: _loading
                                  ? SizedBox(
                                      width: metrics.buttonFontSize * 1.4,
                                      height: metrics.buttonFontSize * 1.4,
                                      child: const CircularProgressIndicator(
                                        strokeWidth: 2,
                                      ),
                                    )
                                  : FittedBox(
                                      fit: BoxFit.scaleDown,
                                      child: Text(
                                        isRegister ? '注册' : '登录',
                                        style: TextStyle(
                                          fontSize: metrics.buttonFontSize,
                                        ),
                                      ),
                                    ),
                            ),
                          ),
                        ),
                      ),
                    ],
                  ),
                ),
              );
            },
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
