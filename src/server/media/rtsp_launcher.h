#pragma once

#include <mutex>
#include <string>

namespace demo::server {

class RtspLauncher {
public:
  RtspLauncher(std::string ffmpeg_path, std::string input_path, std::string rtsp_url, bool required, int pkt_size);

  bool start();
  void stop();
  bool check_alive(std::string& reason);

  std::string ffmpeg_command() const;
  bool required() const { return required_; }

private:
  std::string ffmpeg_path_;
  std::string input_path_;
  std::string rtsp_url_;
  bool required_{false};
  int pkt_size_{1200};

  mutable std::mutex mu_;
  int child_pid_{-1};
};

} // namespace demo::server
