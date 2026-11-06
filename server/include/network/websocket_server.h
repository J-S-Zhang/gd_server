#pragma once

#include <functional>
#include <memory>
#include <string>

namespace guandan {

/// WebSocket 游戏长连接服务
class WebSocketServer {
public:
    using MessageHandler = std::function<void(uint64_t sessionId, const std::string& message)>;  ///< 收到消息
    using ConnectHandler = std::function<void(uint64_t sessionId)>;                             ///< 新连接
    using DisconnectHandler = std::function<void(uint64_t sessionId)>;                         ///< 断开连接

    explicit WebSocketServer(int port);
    ~WebSocketServer();

    /// 设置消息回调
    void setMessageHandler(MessageHandler handler);
    /// 设置连接回调
    void setConnectHandler(ConnectHandler handler);
    /// 设置断开回调
    void setDisconnectHandler(DisconnectHandler handler);

    /// 向指定会话发送文本帧
    void sendTo(uint64_t sessionId, const std::string& message);
    /// 阻塞运行
    void run();
    /// 停止
    void stop();

private:
    int port_;                        ///< 监听端口
    bool running_ = false;            ///< 运行标志
    MessageHandler messageHandler_;   ///< 消息处理
    ConnectHandler connectHandler_;   ///< 连接处理
    DisconnectHandler disconnectHandler_;  ///< 断开处理
};

}  // namespace guandan
