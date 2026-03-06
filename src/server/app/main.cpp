#include "media/rtsp_launcher.h"
#include "network/tcp_sim_server.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace {

struct ServerOptions {
  std::uint16_t tcp_port = 9000;
  std::string video_path = "media/test.mp4";
  std::string rtsp_url = "rtsp://127.0.0.1:8554/live";
  std::string ffmpeg_path = "ffmpeg";
  bool require_rtsp = false;
  int tcp_retry_ms = 2000;
  int rtsp_retry_min_ms = 1000;
  int rtsp_retry_max_ms = 10000;
  int health_interval_sec = 0;

  bool fixed_msg = false;
  double person_conf = 0.92;
  double rod_conf = 0.88;
  double person_cx = 0.36;
  double person_cy = 0.38;
  double person_w = 0.30;
  double person_h = 0.44;
  double rod_cx = 0.38;
  double rod_cy = 0.40;
  double rod_w = 0.28;
  double rod_h = 0.13;
  std::string yolo_txt;
  std::string class_map;
};

struct SubsystemState {
  std::string status{"INIT"};
  int retries{0};
  std::string last_error{""};
};

std::atomic<bool> g_running{true};
std::mutex g_state_mu;
SubsystemState g_tcp_state;
SubsystemState g_rtsp_state;
std::unique_ptr<demo::server::TcpSimServer> g_tcp_server;
demo::server::RtspLauncher* g_rtsp = nullptr;

void print_usage(const char* prog) {
  std::cout << "Usage: " << prog << " [options]\n"
            << "Options:\n"
            << "  --tcp-port <port>            TCP simulator port (default: 9000)\n"
            << "  --video <path>               Input video path (default: media/test.mp4)\n"
            << "  --rtsp-url <url>             RTSP target URL (default: rtsp://127.0.0.1:8554/live)\n"
            << "  --ffmpeg <path>              ffmpeg executable path (default: ffmpeg)\n"
            << "  --require-rtsp               Exit if RTSP pusher startup fails\n"
            << "  --tcp-retry-ms <ms>          TCP bind/listen retry interval (default: 2000)\n"
            << "  --rtsp-retry-min-ms <ms>     RTSP restart min backoff (default: 1000)\n"
            << "  --rtsp-retry-max-ms <ms>     RTSP restart max backoff (default: 10000)\n"
            << "  --health-interval-sec <sec>  Health log interval; 0 disables (default: 0)\n"
            << "  --fixed-msg                  Use fixed detection params (no motion)\n"
            << "  --person-conf <v>            Fixed person confidence (default: 0.92)\n"
            << "  --rod-conf <v>               Fixed rod confidence (default: 0.88)\n"
            << "  --person-cx/cy/w/h <v>       Person bbox normalized params\n"
            << "  --rod-cx/cy/w/h <v>          Rod bbox normalized params\n"
            << "  --yolo-txt <path>            YOLO txt labels: cls cx cy w h conf (one per line)\n"
            << "  --class-map <path>           Class map file, yaml-like: 0: person / 1: rod\n"
            << "  --help                       Show this help\n"
            << "\n"
            << "Compatibility (legacy positional):\n"
            << "  " << prog << " [tcp_port] [video_path]\n\n"
            << "Examples:\n"
            << "  " << prog << " --tcp-port 9000 --video media/test.mp4\n"
            << "  " << prog << " --tcp-port 9000 --health-interval-sec 0\n"
            << "  " << prog << " --fixed-msg --person-conf 0.95 --rod-conf 0.85 --person-cx 0.40 --rod-cx 0.43\n"
            << "  " << prog << " --yolo-txt labels/example_labels.txt --class-map labels/class_map.txt\n"
            << "  " << prog << " 9000 media/test.mp4\n";
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
      if (i + 1 >= argc) return false;
      opts.tcp_port = static_cast<std::uint16_t>(std::stoi(argv[++i]));
      saw_named = true;
      continue;
    }
    if (arg == "--video") {
      if (i + 1 >= argc) return false;
      opts.video_path = argv[++i];
      saw_named = true;
      continue;
    }
    if (arg == "--rtsp-url") {
      if (i + 1 >= argc) return false;
      opts.rtsp_url = argv[++i];
      saw_named = true;
      continue;
    }
    if (arg == "--ffmpeg") {
      if (i + 1 >= argc) return false;
      opts.ffmpeg_path = argv[++i];
      saw_named = true;
      continue;
    }
    if (arg == "--require-rtsp") {
      opts.require_rtsp = true;
      saw_named = true;
      continue;
    }
    if (arg == "--tcp-retry-ms") {
      if (i + 1 >= argc) return false;
      opts.tcp_retry_ms = std::stoi(argv[++i]);
      saw_named = true;
      continue;
    }
    if (arg == "--rtsp-retry-min-ms") {
      if (i + 1 >= argc) return false;
      opts.rtsp_retry_min_ms = std::stoi(argv[++i]);
      saw_named = true;
      continue;
    }
    if (arg == "--rtsp-retry-max-ms") {
      if (i + 1 >= argc) return false;
      opts.rtsp_retry_max_ms = std::stoi(argv[++i]);
      saw_named = true;
      continue;
    }
    if (arg == "--health-interval-sec") {
      if (i + 1 >= argc) return false;
      opts.health_interval_sec = std::stoi(argv[++i]);
      saw_named = true;
      continue;
    }
    if (arg == "--fixed-msg") { opts.fixed_msg = true; saw_named = true; continue; }
    if (arg == "--person-conf") { if (i + 1 >= argc) return false; opts.person_conf = std::stod(argv[++i]); saw_named = true; continue; }
    if (arg == "--rod-conf") { if (i + 1 >= argc) return false; opts.rod_conf = std::stod(argv[++i]); saw_named = true; continue; }
    if (arg == "--person-cx") { if (i + 1 >= argc) return false; opts.person_cx = std::stod(argv[++i]); saw_named = true; continue; }
    if (arg == "--person-cy") { if (i + 1 >= argc) return false; opts.person_cy = std::stod(argv[++i]); saw_named = true; continue; }
    if (arg == "--person-w") { if (i + 1 >= argc) return false; opts.person_w = std::stod(argv[++i]); saw_named = true; continue; }
    if (arg == "--person-h") { if (i + 1 >= argc) return false; opts.person_h = std::stod(argv[++i]); saw_named = true; continue; }
    if (arg == "--rod-cx") { if (i + 1 >= argc) return false; opts.rod_cx = std::stod(argv[++i]); saw_named = true; continue; }
    if (arg == "--rod-cy") { if (i + 1 >= argc) return false; opts.rod_cy = std::stod(argv[++i]); saw_named = true; continue; }
    if (arg == "--rod-w") { if (i + 1 >= argc) return false; opts.rod_w = std::stod(argv[++i]); saw_named = true; continue; }
    if (arg == "--rod-h") { if (i + 1 >= argc) return false; opts.rod_h = std::stod(argv[++i]); saw_named = true; continue; }
    if (arg == "--yolo-txt") { if (i + 1 >= argc) return false; opts.yolo_txt = argv[++i]; saw_named = true; continue; }
    if (arg == "--class-map") { if (i + 1 >= argc) return false; opts.class_map = argv[++i]; saw_named = true; continue; }

    if (!arg.empty() && arg[0] == '-') return false;
    if (saw_named) return false;
    if (i == 1) {
      opts.tcp_port = static_cast<std::uint16_t>(std::stoi(arg));
    } else if (i == 2) {
      opts.video_path = arg;
    } else {
      return false;
    }
  }

  if (opts.tcp_retry_ms < 100) opts.tcp_retry_ms = 100;
  if (opts.rtsp_retry_min_ms < 100) opts.rtsp_retry_min_ms = 100;
  if (opts.rtsp_retry_max_ms < opts.rtsp_retry_min_ms) opts.rtsp_retry_max_ms = opts.rtsp_retry_min_ms;
  if (opts.health_interval_sec < 0) opts.health_interval_sec = 0;
  return true;
}

void update_state(SubsystemState& state, const std::string& status, const std::string& last_error = "", bool retry_inc = false) {
  std::lock_guard<std::mutex> lk(g_state_mu);
  state.status = status;
  if (!last_error.empty()) state.last_error = last_error;
  if (retry_inc) state.retries += 1;
}

void on_signal(int) {
  g_running = false;
  if (g_tcp_server) g_tcp_server->stop();
  if (g_rtsp) g_rtsp->stop();
}

void run_tcp_loop(ServerOptions opts) {
  while (g_running) {
    try {
      update_state(g_tcp_state, "STARTING");
      demo::server::TcpSimServer::SimConfig cfg;
      cfg.fixed = opts.fixed_msg;
      cfg.person_conf = opts.person_conf;
      cfg.rod_conf = opts.rod_conf;
      cfg.person_cx = opts.person_cx;
      cfg.person_cy = opts.person_cy;
      cfg.person_w = opts.person_w;
      cfg.person_h = opts.person_h;
      cfg.rod_cx = opts.rod_cx;
      cfg.rod_cy = opts.rod_cy;
      cfg.rod_w = opts.rod_w;
      cfg.rod_h = opts.rod_h;
      cfg.yolo_txt_path = opts.yolo_txt;
      cfg.class_map_path = opts.class_map;
      auto server = std::make_unique<demo::server::TcpSimServer>(opts.tcp_port, cfg);
      g_tcp_server = std::move(server);
      update_state(g_tcp_state, "RUNNING");
      const int ret = g_tcp_server->run();
      g_tcp_server.reset();

      if (!g_running) break;
      if (ret == 0) {
        update_state(g_tcp_state, "STOPPED");
        continue;
      }

      update_state(g_tcp_state, "RETRY_WAIT", "run() returned non-zero", true);
      std::cerr << "[sim_server][tcp] restarting after failure, retry=" << g_tcp_state.retries
                << " next_wait_ms=" << opts.tcp_retry_ms << "\n";
      std::this_thread::sleep_for(std::chrono::milliseconds(opts.tcp_retry_ms));
    } catch (const std::exception& e) {
      update_state(g_tcp_state, "EXCEPTION", e.what(), true);
      std::cerr << "[sim_server][tcp] EXCEPTION in supervisor: " << e.what() << "\n";
      std::this_thread::sleep_for(std::chrono::milliseconds(opts.tcp_retry_ms));
    } catch (...) {
      update_state(g_tcp_state, "EXCEPTION", "unknown exception", true);
      std::cerr << "[sim_server][tcp] EXCEPTION in supervisor: unknown\n";
      std::this_thread::sleep_for(std::chrono::milliseconds(opts.tcp_retry_ms));
    }
  }
  update_state(g_tcp_state, "STOPPED");
}

void run_rtsp_loop(ServerOptions opts) {
  demo::server::RtspLauncher launcher(opts.ffmpeg_path, opts.video_path, opts.rtsp_url, opts.require_rtsp);
  g_rtsp = &launcher;

  int backoff_ms = opts.rtsp_retry_min_ms;
  while (g_running) {
    try {
      update_state(g_rtsp_state, "STARTING");
      const bool ok = launcher.start();
      if (!ok) {
        if (launcher.required()) {
          update_state(g_rtsp_state, "FAILED_REQUIRED", "startup failed with --require-rtsp", true);
          std::cerr << "[sim_server][rtsp] required startup failed, but process keeps running by design\n";
        } else {
          update_state(g_rtsp_state, "RETRY_WAIT", "startup failed", true);
        }
        std::cerr << "[sim_server][rtsp] startup failed, retry=" << g_rtsp_state.retries
                  << " backoff_ms=" << backoff_ms << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
        backoff_ms = std::min(backoff_ms * 2, opts.rtsp_retry_max_ms);
        continue;
      }

      update_state(g_rtsp_state, "RUNNING");
      backoff_ms = opts.rtsp_retry_min_ms;

      while (g_running) {
        std::string reason;
        if (!launcher.check_alive(reason)) {
          update_state(g_rtsp_state, "RETRY_WAIT", reason, true);
          std::cerr << "[sim_server][rtsp] pusher exited: " << reason << ", retry=" << g_rtsp_state.retries
                    << " backoff_ms=" << backoff_ms << "\n";
          std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
          backoff_ms = std::min(backoff_ms * 2, opts.rtsp_retry_max_ms);
          break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }

      launcher.stop();
    } catch (const std::exception& e) {
      update_state(g_rtsp_state, "EXCEPTION", e.what(), true);
      std::cerr << "[sim_server][rtsp] EXCEPTION in supervisor: " << e.what() << "\n";
      std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
      backoff_ms = std::min(backoff_ms * 2, opts.rtsp_retry_max_ms);
    } catch (...) {
      update_state(g_rtsp_state, "EXCEPTION", "unknown exception", true);
      std::cerr << "[sim_server][rtsp] EXCEPTION in supervisor: unknown\n";
      std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
      backoff_ms = std::min(backoff_ms * 2, opts.rtsp_retry_max_ms);
    }
  }

  launcher.stop();
  g_rtsp = nullptr;
  update_state(g_rtsp_state, "STOPPED");
}

} // namespace

int main(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      print_usage(argv[0]);
      return 0;
    }
  }

  ServerOptions opts;
  try {
    if (!parse_options(argc, argv, opts)) {
      print_usage(argv[0]);
      return 1;
    }
  } catch (const std::exception& e) {
    std::cerr << "Invalid arguments: " << e.what() << "\n";
    print_usage(argv[0]);
    return 1;
  }

  std::signal(SIGINT, on_signal);
  std::signal(SIGTERM, on_signal);


  std::thread tcp_thread(run_tcp_loop, opts);
  std::thread rtsp_thread(run_rtsp_loop, opts);

  while (g_running) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  if (g_tcp_server) g_tcp_server->stop();
  if (g_rtsp) g_rtsp->stop();

  if (tcp_thread.joinable()) tcp_thread.join();
  if (rtsp_thread.joinable()) rtsp_thread.join();

  return 0;
}
