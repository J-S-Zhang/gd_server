#include "utils/logger.h"
#include "voice/voice_server.h"

#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[]) {
    int port = 9002;
    if (argc > 1) {
        port = std::atoi(argv[1]);
    }

    guandan::Logger::info("掼蛋语音服务器启动 UDP " + std::to_string(port));
    guandan::voice::VoiceServer server(port);
    server.run();
    return 0;
}
