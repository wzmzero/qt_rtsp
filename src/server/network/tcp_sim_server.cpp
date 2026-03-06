#include "network/tcp_sim_server.h"

#include "core/timestamp.h"
#include "protocol/messages.h"

#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <random>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace demo::server {

TcpSimServer::TcpSimServer(std::uint16_t port) : port_(port) {}

int TcpSimServer::run() {
  int listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    std::cerr << "socket failed: " << std::strerror(errno) << "\n";
    return 1;
  }

  int opt = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port_);

  if (::bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    std::cerr << "bind failed: " << std::strerror(errno) << "\n";
    ::close(listen_fd);
    return 1;
  }

  if (::listen(listen_fd, 4) < 0) {
    std::cerr << "listen failed: " << std::strerror(errno) << "\n";
    ::close(listen_fd);
    return 1;
  }

  std::cout << "[sim_server] TCP listening on 0.0.0.0:" << port_ << "\n";

  while (running_) {
    sockaddr_in cli{};
    socklen_t len = sizeof(cli);
    int client_fd = ::accept(listen_fd, reinterpret_cast<sockaddr*>(&cli), &len);
    if (client_fd < 0) {
      if (errno == EINTR) continue;
      std::cerr << "accept failed: " << std::strerror(errno) << "\n";
      continue;
    }

    char ip[INET_ADDRSTRLEN]{};
    inet_ntop(AF_INET, &cli.sin_addr, ip, sizeof(ip));
    std::cout << "[sim_server] client connected: " << ip << ":" << ntohs(cli.sin_port) << "\n";

    std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<double> conf(0.35, 0.99);
    std::uniform_int_distribution<int> shift(-30, 30);

    int seq = 0;
    while (running_) {
      const auto now_ms = demo::core::now_ms();

      demo::protocol::Detection d;
      d.label = (seq % 3 == 0) ? "person" : ((seq % 3 == 1) ? "boat" : "none");
      d.confidence = conf(rng);
      d.source_ts_ms = now_ms - 15;

      demo::protocol::Gps g;
      g.time_usec = static_cast<std::int64_t>(now_ms) * 1000;
      g.lat_e7 = 311234567 + shift(rng);
      g.lon_e7 = 1214730000 + shift(rng);
      g.alt_mm = 15000 + seq;
      g.eph_cm = 120;
      g.epv_cm = 180;
      g.vel_cms = 500 + (seq % 80);
      g.cog_cdeg = (seq * 120) % 36000;
      g.fix_type = 3;
      g.satellites_visible = 11;

      const auto line = demo::protocol::to_json_line(d, g, now_ms);
      const auto ret = ::send(client_fd, line.data(), line.size(), MSG_NOSIGNAL);
      if (ret <= 0) {
        std::cerr << "[sim_server] send failed / client disconnected\n";
        break;
      }

      ++seq;
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    ::close(client_fd);
  }

  ::close(listen_fd);
  return 0;
}

void TcpSimServer::stop() { running_ = false; }

} // namespace demo::server
