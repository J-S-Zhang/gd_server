#include "network/http_server.h"
#include "utils/logger.h"

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    using SocketHandle = SOCKET;
    const SocketHandle kInvalidSocket = INVALID_SOCKET;
#else
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <unistd.h>
    using SocketHandle = int;
    const SocketHandle kInvalidSocket = -1;
#endif

#include <cctype>
#include <sstream>
#include <string>
#include <thread>

namespace guandan {

namespace {

bool initNetwork() {
#ifdef _WIN32
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
#else
    return true;
#endif
}

void cleanupNetwork() {
#ifdef _WIN32
    WSACleanup();
#endif
}

void closeSocket(SocketHandle sock) {
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

bool recvLine(SocketHandle sock, std::string& line) {
    char c;
    line.clear();
    while (true) {
        int n = recv(sock, &c, 1, 0);
        if (n <= 0) return false;
        line += c;
        if (line.size() >= 2 && line[line.size() - 2] == '\r' && line[line.size() - 1] == '\n') {
            line.resize(line.size() - 2);
            return true;
        }
    }
}

std::string extractHeaderValue(const std::string& line) {
    auto colon = line.find(':');
    if (colon == std::string::npos) return "";
    std::string value = line.substr(colon + 1);
    while (!value.empty() && value[0] == ' ') value.erase(0, 1);
    return value;
}

std::string extractJsonStringValue(const std::string& json, const std::string& key) {
    const std::string needle = "\"" + key + "\":\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos += needle.size();
    auto end = json.find('"', pos);
    if (end == std::string::npos) return "";
    return json.substr(pos, end - pos);
}

std::string escapeJson(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

std::string buildUserJson(const UserRecord& user, const std::string& token) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"id\":" << user.id;
    oss << ",\"nickname\":\"" << escapeJson(user.nickname) << "\"";
    oss << ",\"username\":\"" << escapeJson(user.nickname) << "\"";
    oss << ",\"token\":\"" << escapeJson(token) << "\"";
    oss << ",\"stats\":{";
    oss << "\"total_games\":" << user.stats.totalGames;
    oss << ",\"wins\":" << user.stats.wins;
    oss << ",\"losses\":" << user.stats.losses;
    oss << "}";
    oss << "}";
    return oss.str();
}

std::string buildErrorJson(const std::string& errorCode, const std::string& message) {
    return "{\"error\":\"" + escapeJson(errorCode) + "\",\"message\":\"" +
           escapeJson(message) + "\"}";
}

void sendHttpResponse(SocketHandle sock, int statusCode, const std::string& statusText,
                      const std::string& body) {
    std::ostringstream resp;
    resp << "HTTP/1.1 " << statusCode << ' ' << statusText << "\r\n";
    resp << "Content-Type: application/json; charset=utf-8\r\n";
    resp << "Access-Control-Allow-Origin: *\r\n";
    resp << "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
    resp << "Access-Control-Allow-Methods: POST, OPTIONS\r\n";
    resp << "Content-Length: " << body.size() << "\r\n";
    resp << "Connection: close\r\n";
    resp << "\r\n";
    resp << body;
    const std::string payload = resp.str();
    ::send(sock, payload.c_str(), static_cast<int>(payload.size()), 0);
}

void handleClient(SocketHandle sock, AuthService* authService) {
    std::string requestLine;
    if (!recvLine(sock, requestLine)) {
        closeSocket(sock);
        return;
    }

    std::string method;
    std::string path;
    {
        std::istringstream iss(requestLine);
        iss >> method >> path;
    }

    int contentLength = 0;
    std::string line;
    while (recvLine(sock, line) && !line.empty()) {
        const auto colon = line.find(':');
        if (colon != std::string::npos) {
            std::string headerName = line.substr(0, colon);
            for (char& c : headerName) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            if (headerName == "content-length") {
                try {
                    contentLength = std::stoi(extractHeaderValue(line));
                } catch (...) {
                    contentLength = 0;
                }
            }
        }
    }

    std::string body;
    body.resize(static_cast<size_t>(contentLength));
    int received = 0;
    while (received < contentLength) {
        int n = recv(sock, &body[received], contentLength - received, 0);
        if (n <= 0) break;
        received += n;
    }
    body.resize(static_cast<size_t>(received));

    if (method == "OPTIONS") {
        sendHttpResponse(sock, 204, "No Content", "");
        closeSocket(sock);
        return;
    }

    if (!authService) {
        sendHttpResponse(sock, 503, "Service Unavailable",
                         buildErrorJson("service_unavailable", "认证服务未就绪"));
        closeSocket(sock);
        return;
    }

    if (method == "POST" && (path == "/api/register" || path == "/register")) {
        const std::string nickname = extractJsonStringValue(body, "nickname");
        const std::string password = extractJsonStringValue(body, "password");
        auto result = authService->registerUser(nickname, password);
        if (result.success) {
            sendHttpResponse(sock, 200, "OK", buildUserJson(result.user, result.token));
        } else if (result.httpStatus == 409) {
            sendHttpResponse(sock, 409, "Conflict", buildErrorJson(result.errorCode, result.message));
        } else {
            sendHttpResponse(sock, result.httpStatus, "Bad Request",
                             buildErrorJson(result.errorCode, result.message));
        }
        closeSocket(sock);
        return;
    }

    if (method == "POST" && (path == "/api/login" || path == "/login")) {
        const std::string nickname = extractJsonStringValue(body, "nickname");
        const std::string password = extractJsonStringValue(body, "password");
        auto result = authService->login(nickname, password);
        if (result.success) {
            sendHttpResponse(sock, 200, "OK", buildUserJson(result.user, result.token));
        } else {
            sendHttpResponse(sock, result.httpStatus, "Unauthorized",
                             buildErrorJson(result.errorCode, result.message));
        }
        closeSocket(sock);
        return;
    }

    sendHttpResponse(sock, 404, "Not Found", buildErrorJson("not_found", "接口不存在"));
    closeSocket(sock);
}

}  // namespace

HttpServer::HttpServer(int port) : port_(port) {}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::run() {
    if (!initNetwork()) {
        Logger::error("HTTP: failed to initialize network");
        return;
    }

    SocketHandle serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == kInvalidSocket) {
        Logger::error("HTTP: failed to create socket");
        return;
    }

    int opt = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(serverSock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        Logger::error("HTTP: failed to bind port " + std::to_string(port_));
        closeSocket(serverSock);
        return;
    }

    if (listen(serverSock, 64) != 0) {
        Logger::error("HTTP: failed to listen");
        closeSocket(serverSock);
        return;
    }

    running_ = true;
    Logger::info("HTTP server listening on port " + std::to_string(port_));

    while (running_) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(serverSock, &readSet);
        timeval tv{1, 0};
        int sel = select(static_cast<int>(serverSock) + 1, &readSet, nullptr, nullptr, &tv);
        if (sel <= 0) continue;

        sockaddr_in clientAddr{};
#ifdef _WIN32
        int addrLen = sizeof(clientAddr);
#else
        socklen_t addrLen = sizeof(clientAddr);
#endif
        SocketHandle clientSock = accept(serverSock,
            reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
        if (clientSock == kInvalidSocket) continue;

        std::thread([this, clientSock]() {
            handleClient(clientSock, authService_);
        }).detach();
    }

    closeSocket(serverSock);
    cleanupNetwork();
}

void HttpServer::stop() {
    running_ = false;
}

}  // namespace guandan
