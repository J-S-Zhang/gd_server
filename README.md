# 六人掼蛋

同时支持 iOS 和 Android 的六人在线掼蛋手机游戏。

## 技术栈

| 层级   | 技术                             |
| ------ | -------------------------------- |
| 客户端 | Flutter + Dart + Riverpod        |
| 服务端 | C++20 + Boost.Asio + Boost.Beast |
| 数据库 | PostgreSQL（MVP 阶段可选）       |
| 通信   | HTTPS + WebSocket Secure (WSS)   |
| 部署   | Linux + Nginx + Docker           |

## 项目结构

```text
gd/
├── client/          # Flutter 手机客户端
├── server/          # C++ 游戏服务器
├── protocol/        # 通信协议文档
├── docs/            # 设计文档
└── deploy/          # 部署配置
```

## 快速开始

### 服务端

```bash
cd server
mkdir build && cd build
cmake ..
cmake --build .
./guandan_server
```

### 客户端

```bash
cd client
flutter pub get
flutter run
```

## 开发顺序

1. 游戏规则文档 → GameEngine → 自动化测试
2. WebSocket 服务器 → Room 系统
3. Flutter 客户端 UI

详见 [六人掼蛋移动端游戏完整实现方案.md](./六人掼蛋移动端游戏完整实现方案.md)
