#include "network/websocket_server.h"
#include "utils/logger.h"

#include <atomic>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/asio.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>

/// @file websocket_server_asio.cpp
/// @brief Boost.Asio/Beast：固定 I/O 线程池 + Session Strand 发送队列。
///        读到的消息仍交给 MessageDispatcher → RoomTaskQueue/RoomExecutor 处理业务。

namespace guandan {

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

using MessagePtr = std::shared_ptr<const std::string>;
using Strand = net::strand<net::io_context::executor_type>;

struct WebSocketServer::Impl {
    int port = 0;
    std::size_t ioThreadCount = 4;
    std::atomic<bool> running{false};

    MessageHandler messageHandler;
    ConnectHandler connectHandler;
    DisconnectHandler disconnectHandler;

    net::io_context ioc{1};
    std::unique_ptr<net::executor_work_guard<net::io_context::executor_type>> workGuard;
    std::unique_ptr<tcp::acceptor> acceptor;
    std::vector<std::thread> workers;

    std::mutex sessionsMutex;
    std::unordered_map<uint64_t, std::weak_ptr<BeastSession>> sessions;
    std::atomic<uint64_t> nextSessionId{1};

    void registerSession(uint64_t id, const std::shared_ptr<BeastSession>& session);
    void unregisterSession(uint64_t id);
    std::shared_ptr<BeastSession> findSession(uint64_t id);
};

class BeastSession : public std::enable_shared_from_this<BeastSession> {
public:
    BeastSession(net::io_context& ioc, tcp::socket socket, WebSocketServer::Impl* server,
                 uint64_t sessionId)
        : strand_(net::make_strand(ioc.get_executor())),
          ws_(strand_),
          server_(server),
          sessionId_(sessionId) {
        // 将已 accept 的 socket 挂到带 strand 的 tcp_stream 上
        beast::get_lowest_layer(ws_).socket() = std::move(socket);
    }

    void start() {
        auto self = shared_from_this();
        ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
        ws_.set_option(websocket::stream_base::decorator(
            [](websocket::response_type& res) {
                res.set(http::field::server, "guandan-asio");
            }));

        ws_.async_accept([self](beast::error_code ec) {
            if (ec) {
                Logger::error("WS handshake failed: " + ec.message());
                return;
            }
            if (self->server_->connectHandler) {
                self->server_->connectHandler(self->sessionId_);
            }
            self->doRead();
        });
    }

    void send(std::string message) {
        auto self = shared_from_this();
        auto msg = std::make_shared<const std::string>(std::move(message));
        net::post(self->strand_, [self, msg = std::move(msg)]() mutable {
            self->enqueueOnStrand(std::move(msg));
        });
    }

    void requestClose() {
        auto self = shared_from_this();
        net::post(self->strand_, [self]() {
            if (self->closed_) return;
            self->closed_ = true;
            beast::error_code ec;
            self->ws_.close(websocket::close_code::normal, ec);
        });
    }

private:
    void enqueueOnStrand(MessagePtr message) {
        if (closed_ || !message) return;
        sendQueue_.push_back(std::move(message));
        if (!writing_) {
            doWrite();
        }
    }

    void doWrite() {
        if (sendQueue_.empty() || closed_) {
            writing_ = false;
            return;
        }
        writing_ = true;
        auto self = shared_from_this();
        ws_.text(true);
        ws_.async_write(
            net::buffer(*sendQueue_.front()),
            [self](beast::error_code ec, std::size_t) {
                if (ec) {
                    self->fail("write", ec);
                    return;
                }
                self->sendQueue_.pop_front();
                if (!self->sendQueue_.empty()) {
                    self->doWrite();
                } else {
                    self->writing_ = false;
                }
            });
    }

    void doRead() {
        auto self = shared_from_this();
        ws_.async_read(buffer_, [self](beast::error_code ec, std::size_t) {
            if (ec == websocket::error::closed) {
                self->onClosed();
                return;
            }
            if (ec) {
                self->fail("read", ec);
                return;
            }

            std::string message = beast::buffers_to_string(self->buffer_.data());
            self->buffer_.consume(self->buffer_.size());

            if (self->server_->messageHandler) {
                self->server_->messageHandler(self->sessionId_, message);
            }
            self->doRead();
        });
    }

    void fail(const char* what, beast::error_code ec) {
        if (!closed_) {
            Logger::error("WS session " + std::to_string(sessionId_) + " " + what + ": " +
                          ec.message());
        }
        onClosed();
    }

    void onClosed() {
        if (closed_) return;
        closed_ = true;
        sendQueue_.clear();
        writing_ = false;
        if (server_->disconnectHandler) {
            server_->disconnectHandler(sessionId_);
        }
        server_->unregisterSession(sessionId_);
    }

    Strand strand_;
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    std::deque<MessagePtr> sendQueue_;
    bool writing_ = false;
    bool closed_ = false;
    WebSocketServer::Impl* server_ = nullptr;
    uint64_t sessionId_ = 0;
};

void WebSocketServer::Impl::registerSession(uint64_t id,
                                            const std::shared_ptr<BeastSession>& session) {
    std::lock_guard lock(sessionsMutex);
    sessions[id] = session;
}

void WebSocketServer::Impl::unregisterSession(uint64_t id) {
    std::lock_guard lock(sessionsMutex);
    sessions.erase(id);
}

std::shared_ptr<BeastSession> WebSocketServer::Impl::findSession(uint64_t id) {
    std::lock_guard lock(sessionsMutex);
    auto it = sessions.find(id);
    if (it == sessions.end()) return nullptr;
    return it->second.lock();
}

WebSocketServer::WebSocketServer(int port, std::size_t ioThreadCount)
    : impl_(std::make_unique<Impl>()) {
    impl_->port = port;
    if (ioThreadCount == 0) {
        const unsigned hc = std::thread::hardware_concurrency();
        impl_->ioThreadCount = hc > 1 ? static_cast<std::size_t>(hc) : 2;
    } else {
        impl_->ioThreadCount = ioThreadCount;
    }
}

WebSocketServer::~WebSocketServer() {
    stop();
}

void WebSocketServer::setMessageHandler(MessageHandler handler) {
    impl_->messageHandler = std::move(handler);
}

void WebSocketServer::setConnectHandler(ConnectHandler handler) {
    impl_->connectHandler = std::move(handler);
}

void WebSocketServer::setDisconnectHandler(DisconnectHandler handler) {
    impl_->disconnectHandler = std::move(handler);
}

void WebSocketServer::sendTo(uint64_t sessionId, const std::string& message) {
    if (auto session = impl_->findSession(sessionId)) {
        session->send(message);
    }
}

void WebSocketServer::run() {
    beast::error_code ec;
    const auto address = net::ip::make_address("0.0.0.0", ec);
    if (ec) {
        Logger::error("Invalid listen address: " + ec.message());
        return;
    }

    impl_->workGuard =
        std::make_unique<net::executor_work_guard<net::io_context::executor_type>>(
            net::make_work_guard(impl_->ioc));

    impl_->acceptor = std::make_unique<tcp::acceptor>(impl_->ioc);
    tcp::endpoint endpoint{address, static_cast<unsigned short>(impl_->port)};
    impl_->acceptor->open(endpoint.protocol(), ec);
    if (ec) {
        Logger::error("acceptor open failed: " + ec.message());
        return;
    }
    impl_->acceptor->set_option(net::socket_base::reuse_address(true), ec);
    impl_->acceptor->bind(endpoint, ec);
    if (ec) {
        Logger::error("bind failed on port " + std::to_string(impl_->port) + ": " + ec.message());
        return;
    }
    impl_->acceptor->listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
        Logger::error("listen failed: " + ec.message());
        return;
    }

    impl_->running = true;
    Logger::info("WebSocket server (Boost.Asio/Beast I/O pool, threads=" +
                 std::to_string(impl_->ioThreadCount) + ") listening on port " +
                 std::to_string(impl_->port));

    auto doAccept = std::make_shared<std::function<void()>>();
    *doAccept = [this, doAccept]() {
        if (!impl_->running.load()) return;
        impl_->acceptor->async_accept(
            [this, doAccept](beast::error_code acceptEc, tcp::socket socket) {
                if (!impl_->running.load()) return;
                if (!acceptEc) {
                    const uint64_t sessionId = impl_->nextSessionId.fetch_add(1);
                    auto session = std::make_shared<BeastSession>(
                        impl_->ioc, std::move(socket), impl_.get(), sessionId);
                    impl_->registerSession(sessionId, session);
                    session->start();
                } else if (acceptEc != net::error::operation_aborted) {
                    Logger::error("async_accept: " + acceptEc.message());
                }
                (*doAccept)();
            });
    };
    (*doAccept)();

    impl_->workers.reserve(impl_->ioThreadCount);
    for (std::size_t i = 0; i < impl_->ioThreadCount; ++i) {
        impl_->workers.emplace_back([this]() { impl_->ioc.run(); });
    }

    for (auto& t : impl_->workers) {
        if (t.joinable()) t.join();
    }
    impl_->workers.clear();
}

void WebSocketServer::stop() {
    if (!impl_ || !impl_->running.exchange(false)) {
        return;
    }

    beast::error_code ec;
    if (impl_->acceptor) {
        impl_->acceptor->close(ec);
    }

    {
        std::vector<std::shared_ptr<BeastSession>> live;
        {
            std::lock_guard lock(impl_->sessionsMutex);
            live.reserve(impl_->sessions.size());
            for (auto& [id, weak] : impl_->sessions) {
                (void)id;
                if (auto s = weak.lock()) live.push_back(std::move(s));
            }
            impl_->sessions.clear();
        }
        for (auto& s : live) {
            s->requestClose();
        }
    }

    if (impl_->workGuard) {
        impl_->workGuard.reset();
    }
    impl_->ioc.stop();
}

}  // namespace guandan
