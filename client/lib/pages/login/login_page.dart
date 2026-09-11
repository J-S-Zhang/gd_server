import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import '../../config/ui_scale.dart';
import '../../controller/auth_controller.dart';
import '../../services/app_update_service.dart';
import '../../theme/game_theme.dart';
import '../../widgets/app_update_dialog.dart';
import '../../widgets/game/game_layout_positioned.dart';

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

  double _regionFont(GameLayoutRect? region, double factor, UiScale ui, double fallback) {
    if (region != null) return region.height * factor;
    return ui.sp(fallback);
  }

  double _regionRadius(GameLayoutRect? region, UiScale ui) {
    if (region != null) return region.height * 0.14;
    return ui.r(ui.config.radius.lg);
  }

  InputDecoration _fieldDecoration(
    UiScale ui,
    GameLayoutRect? region,
    String label,
    IconData icon,
  ) {
    final fieldFont = _regionFont(region, 0.38, ui, ui.config.font.md2);
    final iconSize = _regionFont(region, 0.48, ui, ui.config.font.lg);
    final radius = _regionRadius(region, ui);
    return InputDecoration(
      labelText: label,
      labelStyle: TextStyle(color: Colors.white70, fontSize: fieldFont),
      prefixIcon: Icon(icon, color: Colors.white70, size: iconSize),
      filled: true,
      fillColor: Colors.white.withValues(alpha: 0.12),
      isDense: true,
      contentPadding: EdgeInsets.symmetric(
        horizontal: fieldFont,
        vertical: fieldFont * 0.55,
      ),
      enabledBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(radius),
        borderSide: BorderSide(color: Colors.white.withValues(alpha: 0.2)),
      ),
      focusedBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(radius),
        borderSide: BorderSide(color: GameTheme.accentGold, width: radius * 0.15),
      ),
    );
  }

  Widget _buildTitleHeader(UiScale ui) {
    final region = ui.layoutRect(PageLayoutKind.login, 'logo_header');
    final titleSize = _regionFont(region, 0.55, ui, ui.config.font.title);

    return Center(
      child: Text(
        ui.config.login.title,
        style: TextStyle(
          color: Colors.white,
          fontSize: titleSize,
          fontWeight: FontWeight.bold,
        ),
        textAlign: TextAlign.center,
      ),
    );
  }

  Widget _buildTabBar(UiScale ui) {
    final region = ui.layoutRect(PageLayoutKind.login, 'tab_bar');
    final tabFont = _regionFont(region, 0.55, ui, ui.config.font.md2);
    final radius = _regionRadius(region, ui);

    return Container(
      width: double.infinity,
      decoration: GameTheme.panelDecoration(ui, radius: radius),
      child: TabBar(
        controller: _tabController,
        indicatorColor: GameTheme.accentGold,
        labelColor: Colors.white,
        unselectedLabelColor: Colors.white54,
        labelStyle: TextStyle(fontSize: tabFont),
        unselectedLabelStyle: TextStyle(fontSize: tabFont),
        onTap: (_) => setState(() {}),
        tabs: const [
          Tab(text: '登录'),
          Tab(text: '注册'),
        ],
      ),
    );
  }

  Widget _buildTextField({
    required UiScale ui,
    required String elementId,
    required TextEditingController controller,
    required String label,
    required IconData icon,
    bool obscure = false,
  }) {
    final region = ui.layoutRect(PageLayoutKind.login, elementId);
    final fieldFont = _regionFont(region, 0.38, ui, ui.config.font.md2);

    return TextField(
      controller: controller,
      obscureText: obscure,
      maxLines: 1,
      style: TextStyle(color: Colors.white, fontSize: fieldFont),
      decoration: _fieldDecoration(ui, region, label, icon),
    );
  }

  Widget _buildSubmitButton(UiScale ui, bool isRegister) {
    final region = ui.layoutRect(PageLayoutKind.login, 'submit_button');
    final btnH = region?.height ?? ui.h(ui.config.layout.loginButtonHeight);
    final btnFont = _regionFont(region, 0.38, ui, ui.config.font.lg);

    return SizedBox(
      width: double.infinity,
      height: btnH,
      child: ElevatedButton(
        onPressed: _loading ? null : (isRegister ? _submitRegister : _submitLogin),
        style: GameTheme.playButtonStyle(ui).copyWith(
          minimumSize: WidgetStateProperty.all(Size(double.infinity, btnH)),
          padding: WidgetStateProperty.all(
            EdgeInsets.symmetric(vertical: btnFont * 0.3),
          ),
        ),
        child: _loading
            ? SizedBox(
                width: btnFont * 1.4,
                height: btnFont * 1.4,
                child: const CircularProgressIndicator(strokeWidth: 2),
              )
            : FittedBox(
                fit: BoxFit.scaleDown,
                child: Text(
                  isRegister ? '注册' : '登录',
                  style: TextStyle(fontSize: btnFont),
                ),
              ),
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
          child: Stack(
            fit: StackFit.expand,
            clipBehavior: Clip.hardEdge,
            children: [
              PageLayoutPositioned(
                page: PageLayoutKind.login,
                elementId: 'logo_header',
                child: _buildTitleHeader(ui),
              ),
              PageLayoutPositioned(
                page: PageLayoutKind.login,
                elementId: 'tab_bar',
                child: _buildTabBar(ui),
              ),
              PageLayoutPositioned(
                page: PageLayoutKind.login,
                elementId: 'nickname_field',
                child: _buildTextField(
                  ui: ui,
                  elementId: 'nickname_field',
                  controller: _nicknameController,
                  label: '昵称',
                  icon: Icons.person,
                ),
              ),
              PageLayoutPositioned(
                page: PageLayoutKind.login,
                elementId: 'password_field',
                child: _buildTextField(
                  ui: ui,
                  elementId: 'password_field',
                  controller: _passwordController,
                  label: '密码',
                  icon: Icons.lock,
                  obscure: true,
                ),
              ),
              if (isRegister)
                PageLayoutPositioned(
                  page: PageLayoutKind.login,
                  elementId: 'confirm_field',
                  child: _buildTextField(
                    ui: ui,
                    elementId: 'confirm_field',
                    controller: _confirmPasswordController,
                    label: '确认密码',
                    icon: Icons.lock_outline,
                    obscure: true,
                  ),
                ),
              PageLayoutPositioned(
                page: PageLayoutKind.login,
                elementId: 'submit_button',
                child: _buildSubmitButton(ui, isRegister),
              ),
            ],
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
