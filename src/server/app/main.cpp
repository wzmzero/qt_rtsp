#include "media/rtsp_launcher.h"
#include "network/tcp_sim_server.h"

#include <csignal>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <string>

namespace {

struct ServerOptions {
  std::uint16_t tcp_port = 9000;
  std::string video_path = "media/test.mp4";
  std::string rtsp_url = "rtsp://127.0.0.1:8554/live";
  std::string ffmpeg_path = "ffmpeg";
  bool require_rtsp = false;
};

std::unique_ptr<demo::server::TcpSimServer> g_server;
demo::server::RtspLauncher* g_rtsp = nullptr;

void print_usage(const char* prog) {
  std::cout << "Usage: " << prog << " [options]\n"
            << "Options:\n"
            << "  --tcp-port <port>       TCP simulator port (default: 9000)\n"
            << "  --video <path>          Input video path (default: media/test.mp4)\n"
            << "  --rtsp-url <url>        RTSP target URL (default: rtsp://127.0.0.1:8554/live)\n"
            << "  --ffmpeg <path>         ffmpeg executable path (default: ffmpeg)\n"
            << "  --require-rtsp          Exit if RTSP pusher startup fails\n"
            << "  --help                  Show this help\n"
            << "\n"
            << "Compatibility (legacy positional):\n"
            << "  " << prog << " [tcp_port] [video_path]\n";
}

bool parse_options(int argc, char** argv, ServerOptions& opts) {
  bool saw_named = false;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      print_usage(argv[0]);
      return false;
    }
    if (arg == "--tcp-port") {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for --tcp-port\n";
        return false;
      }
      opts.tcp_port = static_cast<std::uint16_t>(std::stoi(argv[++i]));
      saw_named = true;
      continue;
    }
    if (arg == "--video") {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for --video\n";
        return false;
      }
      opts.video_path = argv[++i];
      saw_named = true;
      continue;
    }
    if (arg == "--rtsp-url") {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for --rtsp-url\n";
        return false;
      }
      opts.rtsp_url = argv[++i];
      saw_named = true;
      continue;
    }
    if (arg == "--ffmpeg") {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for --ffmpeg\n";
        return false;
      }
      opts.ffmpeg_path = argv[++i];
      saw_named = true;
      continue;
    }
    if (arg == "--require-rtsp") {
      opts.require_rtsp = true;
      saw_named = true;
      continue;
    }

    if (!arg.empty() && arg[0] == '-') {
      std::cerr << "Unknown option: " << arg << "\n";
      return false;
    }

    if (saw_named) {
      std::cerr << "Positional arguments cannot be mixed with named options: " << arg << "\n";
      return false;
    }

    // legacy positional mode: [tcp_port] [video_path]
    if (i == 1) {
      opts.tcp_port = static_cast<std::uint16_t>(std::stoi(arg));
    } else if (i == 2) {
      opts.video_path = arg;
    } else {
      std::cerr << "Too many positional arguments\n";
      return false;
    }
  }

  return true;
}

void on_sigint(int) {
  if (g_server) g_server->stop();
  if (g_rtsp) g_rtsp->stop();
}

} // namespace

int main(int argc, char** argv) {
  ServerOptions opts;
  try {
    if (!parse_options(argc, argv, opts)) {
      return 1;
    }
  } catch (const std::exception& e) {
    std::cerr << "Invalid arguments: " << e.what() << "\n";
    print_usage(argv[0]);
    return 1;
  }

  demo::server::RtspLauncher launcher(opts.ffmpeg_path, opts.video_path, opts.rtsp_url, opts.require_rtsp);
  g_rtsp = &launcher;

  std::signal(SIGINT, on_sigint);
  std::signal(SIGTERM, on_sigint);

  const bool rtsp_ok = launcher.start();
  if (!rtsp_ok) {
    if (launcher.required()) {
      std::cerr << "[sim_server] RTSP startup failed and --require-rtsp is set, exiting.\n";
      return 2;
    }
    std::cerr << "[sim_server] WARN: RTSP startup failed, TCP simulator will continue running.\n";
  }

  g_server = std::make_unique<demo::server::TcpSimServer>(opts.tcp_port);
  std::cout << "[sim_server] Start telemetry simulator on port " << opts.tcp_port << "\n";

  const int ret = g_server->run();
  launcher.stop();
  g_rtsp = nullptr;
  return ret;
}
