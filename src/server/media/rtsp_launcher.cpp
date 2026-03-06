#include "media/rtsp_launcher.h"

#include <cerrno>
#include <csignal>
#include <cstring>
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace demo::server {

RtspLauncher::RtspLauncher(std::string ffmpeg_path, std::string input_path, std::string rtsp_url, bool required)
    : ffmpeg_path_(std::move(ffmpeg_path)), input_path_(std::move(input_path)), rtsp_url_(std::move(rtsp_url)),
      required_(required) {}

std::string RtspLauncher::ffmpeg_command() const {
  std::ostringstream oss;
  oss << ffmpeg_path_ << " -re -stream_loop -1 -i \"" << input_path_
      << "\" -an -c:v libx264 -preset veryfast -tune zerolatency "
      << "-f rtsp -rtsp_transport tcp \"" << rtsp_url_ << "\"";
  return oss.str();
}

bool RtspLauncher::start() {
  if (child_pid_ > 0) {
    return true;
  }

  std::cout << "[sim_server] Starting RTSP pusher:\n  " << ffmpeg_command() << "\n";

  const pid_t pid = ::fork();
  if (pid < 0) {
    std::cerr << "[sim_server] ERROR: fork ffmpeg process failed: " << std::strerror(errno) << "\n";
    return false;
  }

  if (pid == 0) {
    std::vector<char*> args;
    args.push_back(const_cast<char*>(ffmpeg_path_.c_str()));
    args.push_back(const_cast<char*>("-re"));
    args.push_back(const_cast<char*>("-stream_loop"));
    args.push_back(const_cast<char*>("-1"));
    args.push_back(const_cast<char*>("-i"));
    args.push_back(const_cast<char*>(input_path_.c_str()));
    args.push_back(const_cast<char*>("-an"));
    args.push_back(const_cast<char*>("-c:v"));
    args.push_back(const_cast<char*>("libx264"));
    args.push_back(const_cast<char*>("-preset"));
    args.push_back(const_cast<char*>("veryfast"));
    args.push_back(const_cast<char*>("-tune"));
    args.push_back(const_cast<char*>("zerolatency"));
    args.push_back(const_cast<char*>("-f"));
    args.push_back(const_cast<char*>("rtsp"));
    args.push_back(const_cast<char*>("-rtsp_transport"));
    args.push_back(const_cast<char*>("tcp"));
    args.push_back(const_cast<char*>(rtsp_url_.c_str()));
    args.push_back(nullptr);

    ::execvp(ffmpeg_path_.c_str(), args.data());

    std::cerr << "[sim_server] ERROR: exec ffmpeg failed: " << std::strerror(errno) << "\n";
    _exit(127);
  }

  child_pid_ = static_cast<int>(pid);

  int status = 0;
  for (int i = 0; i < 8; ++i) {
    const pid_t ret = ::waitpid(pid, &status, WNOHANG);
    if (ret == 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }
    if (ret == pid) {
      child_pid_ = -1;
      if (WIFEXITED(status)) {
        std::cerr << "[sim_server] ERROR: ffmpeg exited immediately with code " << WEXITSTATUS(status)
                  << ". Please check ffmpeg path/input/RTSP URL.\n";
      } else if (WIFSIGNALED(status)) {
        std::cerr << "[sim_server] ERROR: ffmpeg terminated by signal " << WTERMSIG(status) << ".\n";
      } else {
        std::cerr << "[sim_server] ERROR: ffmpeg exited unexpectedly.\n";
      }
      return false;
    }

    std::cerr << "[sim_server] ERROR: waitpid failed while checking ffmpeg startup: " << std::strerror(errno) << "\n";
    child_pid_ = -1;
    return false;
  }

  std::cout << "[sim_server] RTSP pusher started (pid=" << child_pid_ << ")\n";
  return true;
}

void RtspLauncher::stop() {
  if (child_pid_ <= 0) {
    return;
  }

  const pid_t pid = static_cast<pid_t>(child_pid_);

  if (::kill(pid, SIGTERM) != 0 && errno != ESRCH) {
    std::cerr << "[sim_server] WARN: failed to SIGTERM ffmpeg(pid=" << child_pid_ << "): " << std::strerror(errno)
              << "\n";
  }

  int status = 0;
  bool exited = false;
  for (int i = 0; i < 20; ++i) {
    const pid_t ret = ::waitpid(pid, &status, WNOHANG);
    if (ret == pid) {
      exited = true;
      break;
    }
    if (ret == 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }
    if (errno == ECHILD) {
      exited = true;
      break;
    }
    break;
  }

  if (!exited) {
    std::cerr << "[sim_server] WARN: ffmpeg did not exit in time, sending SIGKILL (pid=" << child_pid_ << ")\n";
    if (::kill(pid, SIGKILL) == 0) {
      (void)::waitpid(pid, &status, 0);
    }
  }

  std::cout << "[sim_server] RTSP pusher stopped\n";
  child_pid_ = -1;
}

} // namespace demo::server
