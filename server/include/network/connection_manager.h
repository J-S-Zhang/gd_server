#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

#ifdef _WIN32
#include <winsock2.h>
using SocketHandle = SOCKET;
#else
using SocketHandle = int;
#endif

namespace guandan {

/// 底层 TCP/WebSocket 连接描述
struct Connection {
    uint64_t sessionId = 0;   ///< 关联会话 id
    SocketHandle socket = 0;  ///< 套接字
    bool isWebSocket = false; ///< 是否已完成 WebSocket 握手
};

/// 按 sessionId 管理连接并发送数据
class ConnectionManager {
public:
    /// 登记连接
    void add(uint64_t sessionId, SocketHandle socket);
    /// 移除连接
    void remove(uint64_t sessionId);
    /// 获取连接，不存在返回 nullptr
    Connection* get(uint64_t sessionId);
    /// 向会话发送消息，成功返回 true
    bool send(uint64_t sessionId, const std::string& message);

private:
    std::mutex mutex_;                                      ///< 保护 connections_
    std::unordered_map<uint64_t, Connection> connections_;  ///< 会话 -> 连接
};

}  // namespace guandan
