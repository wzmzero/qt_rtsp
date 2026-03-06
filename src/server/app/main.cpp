#include "media/rtsp_launcher.h"
#include "network/tcp_sim_server.h"

#include <csignal>
#include <iostream>
#include <memory>

namespace {
std::unique_ptr<demo::server::TcpSimServer> g_server;

void on_sigint(int) {
  if (g_server) g_server->stop();
}
} // namespace

int main(int argc, char** argv) {
  const std::uint16_t tcp_port = (argc > 1) ? static_cast<std::uint16_t>(std::stoi(argv[1])) : 9000;
  const std::string sample = (argc > 2) ? argv[2] : "sample.mp4";

  demo::server::RtspLauncher launcher(sample, "rtsp://127.0.0.1:8554/live");
  launcher.print_hint();

  std::signal(SIGINT, on_sigint);

  g_server = std::make_unique<demo::server::TcpSimServer>(tcp_port);
  std::cout << "[sim_server] Start telemetry simulator on port " << tcp_port << "\n";
  return g_server->run();
}
