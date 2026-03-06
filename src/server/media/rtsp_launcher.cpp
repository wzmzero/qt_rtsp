#include "media/rtsp_launcher.h"

#include <iostream>
#include <sstream>

namespace demo::server {

RtspLauncher::RtspLauncher(std::string input_path, std::string rtsp_url)
    : input_path_(std::move(input_path)), rtsp_url_(std::move(rtsp_url)) {}

std::string RtspLauncher::ffmpeg_command() const {
  std::ostringstream oss;
  oss << "ffmpeg -re -stream_loop -1 -i \"" << input_path_
      << "\" -an -c:v libx264 -preset veryfast -tune zerolatency "
      << "-f rtsp -rtsp_transport tcp \"" << rtsp_url_ << "\"";
  return oss.str();
}

void RtspLauncher::print_hint() const {
  std::cout << "[sim_server] RTSP push (manual) example:\n"
            << "  " << ffmpeg_command() << "\n"
            << "[sim_server] Default endpoint suggestion: rtsp://127.0.0.1:8554/live\n";
}

} // namespace demo::server
