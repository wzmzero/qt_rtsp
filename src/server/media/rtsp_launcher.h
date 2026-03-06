#pragma once

#include <string>

namespace demo::server {

class RtspLauncher {
public:
  RtspLauncher(std::string ffmpeg_path, std::string input_path, std::string rtsp_url, bool required);

  bool start();
  void stop();

  std::string ffmpeg_command() const;
  bool required() const { return required_; }

private:
  std::string ffmpeg_path_;
  std::string input_path_;
  std::string rtsp_url_;
  bool required_{false};
  int child_pid_{-1};
};

} // namespace demo::server
