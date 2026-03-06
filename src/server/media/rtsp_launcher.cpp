#include "media/rtsp_launcher.h"

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
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
  {
    std::lock_guard<std::mutex> lk(mu_);
    if (child_pid_ > 0) {
      return true;
    }
  }

  std::cout << "[sim_server][rtsp] starting pusher:\n  " << ffmpeg_command() << "\n";

  const pid_t pid = ::fork();
  if (pid < 0) {
    std::cerr << "[sim_server][rtsp] ERROR: fork ffmpeg failed: " << std::strerror(errno) << "\n";
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

    std::cerr << "[sim_server][rtsp] ERROR: exec ffmpeg failed: " << std::strerror(errno) << "\n";
    _exit(127);
  }

  {
    std::lock_guard<std::mutex> lk(mu_);
    child_pid_ = static_cast<int>(pid);
  }

  int status = 0;
  for (int i = 0; i < 8; ++i) {
    const pid_t ret = ::waitpid(pid, &status, WNOHANG);
    if (ret == 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }
    if (ret == pid) {
      std::lock_guard<std::mutex> lk(mu_);
      child_pid_ = -1;
      if (WIFEXITED(status)) {
        std::cerr << "[sim_server][rtsp] ERROR: ffmpeg exited quickly with code " << WEXITSTATUS(status) << "\n";
      } else if (WIFSIGNALED(status)) {
        std::cerr << "[sim_server][rtsp] ERROR: ffmpeg terminated by signal " << WTERMSIG(status) << "\n";
      } else {
        std::cerr << "[sim_server][rtsp] ERROR: ffmpeg exited unexpectedly\n";
      }
      return false;
    }

    std::cerr << "[sim_server][rtsp] ERROR: waitpid startup check failed: " << std::strerror(errno) << "\n";
    std::lock_guard<std::mutex> lk(mu_);
    child_pid_ = -1;
    return false;
  }

  std::cout << "[sim_server][rtsp] pusher started (pid=" << pid << ")\n";
  return true;
}

bool RtspLauncher::check_alive(std::string& reason) {
  pid_t pid = -1;
  {
    std::lock_guard<std::mutex> lk(mu_);
    pid = child_pid_;
  }
  if (pid <= 0) {
    reason = "ffmpeg not running";
    return false;
  }

  int status = 0;
  const pid_t ret = ::waitpid(pid, &status, WNOHANG);
  if (ret == 0) {
    return true;
  }

  {
    std::lock_guard<std::mutex> lk(mu_);
    if (child_pid_ == pid) child_pid_ = -1;
  }

  if (ret == pid) {
    if (WIFEXITED(status)) {
      reason = "ffmpeg exited with code " + std::to_string(WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
      reason = "ffmpeg killed by signal " + std::to_string(WTERMSIG(status));
    } else {
      reason = "ffmpeg exited unexpectedly";
    }
    return false;
  }

  if (errno == ECHILD) {
    reason = "ffmpeg child already reaped";
    return false;
  }

  reason = std::string("waitpid failed: ") + std::strerror(errno);
  return false;
}

void RtspLauncher::stop() {
  pid_t pid = -1;
  {
    std::lock_guard<std::mutex> lk(mu_);
    if (child_pid_ <= 0) {
      return;
    }
    pid = static_cast<pid_t>(child_pid_);
  }

  if (::kill(pid, SIGTERM) != 0 && errno != ESRCH) {
    std::cerr << "[sim_server][rtsp] WARN: SIGTERM ffmpeg(" << pid << ") failed: " << std::strerror(errno) << "\n";
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
    std::cerr << "[sim_server][rtsp] WARN: ffmpeg did not exit in time, sending SIGKILL (pid=" << pid << ")\n";
    if (::kill(pid, SIGKILL) == 0) {
      (void)::waitpid(pid, &status, 0);
    }
  }

  {
    std::lock_guard<std::mutex> lk(mu_);
    child_pid_ = -1;
  }
  std::cout << "[sim_server][rtsp] pusher stopped\n";
}

} // namespace demo::server
