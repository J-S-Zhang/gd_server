#include "auth/auth_service.h"
#include "network/http_server.h"
#include "network/message_dispatcher.h"
#include "network/session_manager.h"
#include "network/websocket_server.h"
#include "room/room_manager.h"
#include "timer/timer_manager.h"
#include "user/user_store.h"
#include "user/avatar_storage.h"
#include "auth/token_manager.h"
#include "utils/logger.h"
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char* argv[]) {
    int wsPort = 9001;
    int httpPort = 8080;
    std::string dbPath = "users.json";
    std::string appVersionPath = "server/config/app_version.json";
    std::string avatarsDir = "downloads/avatars";
    if (argc > 1) {
        wsPort = std::atoi(argv[1]);
    }
    if (argc > 2) {
        httpPort = std::atoi(argv[2]);
    }
    if (argc > 3) {
        dbPath = argv[3];
    }
    if (argc > 4) {
        appVersionPath = argv[4];
    }
    if (argc > 5) {
        avatarsDir = argv[5];
    }

    guandan::Logger::info("六人掼蛋游戏服务器启动");
    guandan::Logger::info("规则版本: six_player_guandan_v1");
    guandan::Logger::info("WebSocket 端口: " + std::to_string(wsPort));
    guandan::Logger::info("HTTP 端口: " + std::to_string(httpPort));
    guandan::Logger::info("用户数据库: " + dbPath);
    guandan::Logger::info("版本配置: " + appVersionPath);
    guandan::Logger::info("头像目录: " + avatarsDir);

    guandan::AppVersionInfo appVersionInfo;
    if (!guandan::loadAppVersionInfo(appVersionPath, appVersionInfo)) {
        guandan::Logger::info("使用默认 App 版本配置");
    }

    guandan::UserStore userStore(dbPath);
    guandan::TokenManager tokenManager;
    guandan::AuthService authService(userStore, tokenManager);

    guandan::RoomManager roomManager;
    guandan::SessionManager sessionManager;
    guandan::TimerManager timerManager;
    guandan::MessageDispatcher dispatcher(roomManager, sessionManager, timerManager, authService);

    guandan::HttpServer httpServer(httpPort);
    httpServer.setAuthService(&authService);
    httpServer.setAppVersionInfo(appVersionInfo);
    guandan::AvatarStorageConfig avatarConfig;
    avatarConfig.storageDir = avatarsDir;
    avatarConfig.publicPathPrefix = "/downloads/avatars";
    httpServer.setAvatarStorageConfig(avatarConfig);
    std::thread httpThread([&]() { httpServer.run(); });

    guandan::WebSocketServer server(wsPort);

    sessionManager.setSendCallback([&](uint64_t sessionId, const std::string& message) {
        server.sendTo(sessionId, message);
    });

    server.setConnectHandler([&](uint64_t sessionId) {
        sessionManager.ensureSession(sessionId);
        guandan::Logger::info("Client connected, session " + std::to_string(sessionId));
    });

    server.setDisconnectHandler([&](uint64_t sessionId) {
        guandan::Logger::info("Client disconnected, session " + std::to_string(sessionId));
        sessionManager.removeSession(sessionId);
    });

    server.setMessageHandler([&](uint64_t sessionId, const std::string& message) {
        dispatcher.dispatch(sessionId, message, [&](const std::string& response) {
            server.sendTo(sessionId, response);
        });
    });

    // Timer tick thread
    std::atomic<bool> timerRunning{true};
    std::thread timerThread([&]() {
        while (timerRunning) {
            timerManager.tick();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });

    server.run();

    httpServer.stop();
    if (httpThread.joinable()) httpThread.join();

    timerRunning = false;
    if (timerThread.joinable()) timerThread.join();

    guandan::Logger::info("服务器已停止");
    return 0;
}
