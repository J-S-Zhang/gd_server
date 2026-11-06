#pragma once

namespace guandan::voice {

/// UDP 语音中继服务（收包转发同房间 peer）
class VoiceServer {
public:
    explicit VoiceServer(int port);
    ~VoiceServer();

    /// 阻塞运行
    void run();
    /// 停止
    void stop();

private:
    int port_ = 0;        ///< 监听端口
    bool running_ = false;  ///< 运行标志
};

}  // namespace guandan::voice
