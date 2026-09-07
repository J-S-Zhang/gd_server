#pragma once

#include <functional>
#include <memory>
#include <string>

namespace guandan {

class WebSocketServer {
public:
    using MessageHandler = std::function<void(uint64_t sessionId, const std::string& message)>;
    using ConnectHandler = std::function<void(uint64_t sessionId)>;
    using DisconnectHandler = std::function<void(uint64_t sessionId)>;

    explicit WebSocketServer(int port);
    ~WebSocketServer();

    void setMessageHandler(MessageHandler handler);
    void setConnectHandler(ConnectHandler handler);
    void setDisconnectHandler(DisconnectHandler handler);

    void sendTo(uint64_t sessionId, const std::string& message);
    void run();
    void stop();

private:
    int port_;
    bool running_ = false;
    MessageHandler messageHandler_;
    ConnectHandler connectHandler_;
    DisconnectHandler disconnectHandler_;
};

}  // namespace guandan
