#pragma once

#include <string>

namespace demo::server {

class RtspLauncher {
public:
  RtspLauncher(std::string input_path, std::string rtsp_url);
  std::string ffmpeg_command() const;
  void print_hint() const;

private:
  std::string input_path_;
  std::string rtsp_url_;
};

} // namespace demo::server
