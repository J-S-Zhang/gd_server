#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>

namespace guandan {

class BeastSession;  // Asio 实现中的会话（需访问 Impl）

/// WebSocket 游戏长连接服务
///
/// 编译开关 GUANDAN_USE_BOOST（CMake: GUANDAN_ENABLE_BOOST）：
/// - OFF：原生 socket，一连接一线程（websocket_server.cpp）
/// - ON ：Boost.Asio/Beast，固定 I/O 线程池（websocket_server_asio.cpp）
class WebSocketServer {
    friend class BeastSession;

public:
    using MessageHandler = std::function<void(uint64_t sessionId, const std::string& message)>;  ///< 收到消息
    using ConnectHandler = std::function<void(uint64_t sessionId)>;                             ///< 新连接
    using DisconnectHandler = std::function<void(uint64_t sessionId)>;                         ///< 断开连接

    /// \param port 监听端口
    /// \param ioThreadCount Asio 模式下 I/O 工作线程数；0 表示按 CPU 核数估算（至少 2）。
    ///                      原生 socket 模式忽略此参数。
    explicit WebSocketServer(int port, std::size_t ioThreadCount = 0);
    ~WebSocketServer();

    WebSocketServer(const WebSocketServer&) = delete;
    WebSocketServer& operator=(const WebSocketServer&) = delete;

    /// 设置消息回调
    void setMessageHandler(MessageHandler handler);
    /// 设置连接回调
    void setConnectHandler(ConnectHandler handler);
    /// 设置断开回调
    void setDisconnectHandler(DisconnectHandler handler);

    /// 向指定会话发送文本帧（可跨线程调用）
    void sendTo(uint64_t sessionId, const std::string& message);
    /// 阻塞运行（直到 stop）
    void run();
    /// 停止
    void stop();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace guandan
