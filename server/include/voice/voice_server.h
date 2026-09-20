#pragma once

namespace guandan::voice {

class VoiceServer {
public:
    explicit VoiceServer(int port);
    ~VoiceServer();

    void run();
    void stop();

private:
    int port_ = 0;
    bool running_ = false;
};

}  // namespace guandan::voice
