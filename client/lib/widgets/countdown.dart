import 'dart:async';
import 'dart:math' as math;
import 'package:flutter/material.dart';
import '../config/ui_scale.dart';
import '../theme/game_theme.dart';

class CountdownWidget extends StatefulWidget {
  final int seconds;
  final bool circular;

  const CountdownWidget({
    super.key,
    required this.seconds,
    this.circular = false,
  });

  @override
  State<CountdownWidget> createState() => _CountdownWidgetState();
}

class _CountdownWidgetState extends State<CountdownWidget> {
  late int _remaining;
  Timer? _timer;

  @override
  void initState() {
    super.initState();
    _remaining = widget.seconds;
    _timer = Timer.periodic(const Duration(seconds: 1), (_) {
      if (_remaining > 0) {
        setState(() => _remaining--);
      }
    });
  }

  @override
  void didUpdateWidget(covariant CountdownWidget oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.seconds != widget.seconds) {
      _timer?.cancel();
      _remaining = widget.seconds;
      _timer = Timer.periodic(const Duration(seconds: 1), (_) {
        if (_remaining > 0) {
          setState(() => _remaining--);
        }
      });
    }
  }

  @override
  void dispose() {
    _timer?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final ui = context.ui;
    final color = _remaining <= 10 ? Colors.redAccent : GameTheme.accentGold;

    if (widget.circular) {
      final size = ui.w(ui.config.countdown.circularSize);
      final progress = widget.seconds <= 0 ? 0.0 : _remaining / widget.seconds;
      return SizedBox(
        width: size,
        height: size,
        child: Stack(
          alignment: Alignment.center,
          children: [
            CustomPaint(
              size: Size(size, size),
              painter: _RingPainter(
                progress: progress,
                color: color,
                strokeWidth: ui.r(ui.config.countdown.strokeWidth),
              ),
            ),
            Text(
              '$_remaining',
              style: TextStyle(
                color: color,
                fontWeight: FontWeight.bold,
                fontSize: ui.sp(ui.config.font.md2),
              ),
            ),
          ],
        ),
      );
    }

    return Container(
      padding: ui.edgeInsetsSymmetric(horizontal: ui.config.spacing.lg, vertical: ui.config.spacing.sm),
      decoration: BoxDecoration(
        color: color.withValues(alpha: 0.2),
        borderRadius: BorderRadius.circular(ui.r(ui.config.radius.xl)),
        border: Border.all(color: color),
      ),
      child: Text(
        '$_remaining s',
        style: TextStyle(color: color, fontWeight: FontWeight.bold, fontSize: ui.sp(ui.config.font.md)),
      ),
    );
  }
}

class _RingPainter extends CustomPainter {
  final double progress;
  final Color color;
  final double strokeWidth;

  _RingPainter({
    required this.progress,
    required this.color,
    required this.strokeWidth,
  });

  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(size.width / 2, size.height / 2);
    final radius = size.width / 2 - strokeWidth;
    final bgPaint = Paint()
      ..color = Colors.white.withValues(alpha: 0.15)
      ..style = PaintingStyle.stroke
      ..strokeWidth = strokeWidth;
    final fgPaint = Paint()
      ..color = color
      ..style = PaintingStyle.stroke
      ..strokeWidth = strokeWidth
      ..strokeCap = StrokeCap.round;

    canvas.drawCircle(center, radius, bgPaint);
    canvas.drawArc(
      Rect.fromCircle(center: center, radius: radius),
      -math.pi / 2,
      2 * math.pi * progress,
      false,
      fgPaint,
    );
  }

  @override
  bool shouldRepaint(covariant _RingPainter oldDelegate) {
    return oldDelegate.progress != progress ||
        oldDelegate.color != color ||
        oldDelegate.strokeWidth != strokeWidth;
  }
}
