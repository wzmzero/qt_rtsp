#include "network/tcp_sim_server.h"

#include "core/timestamp.h"
#include "protocol/messages.h"

#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <exception>
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
  try {
    running_ = true;

    int listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
      std::cerr << "[sim_server][tcp] socket failed: " << std::strerror(errno) << "\n";
      return 1;
    }
    listen_fd_ = listen_fd;

    int opt = 1;
    (void)setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (::bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
      std::cerr << "[sim_server][tcp] bind failed: " << std::strerror(errno) << "\n";
      ::close(listen_fd);
      listen_fd_ = -1;
      return 1;
    }

    if (::listen(listen_fd, 4) < 0) {
      std::cerr << "[sim_server][tcp] listen failed: " << std::strerror(errno) << "\n";
      ::close(listen_fd);
      listen_fd_ = -1;
      return 1;
    }

    std::cout << "[sim_server][tcp] listening on 0.0.0.0:" << port_ << "\n";

    while (running_) {
      sockaddr_in cli{};
      socklen_t len = sizeof(cli);
      int client_fd = ::accept(listen_fd, reinterpret_cast<sockaddr*>(&cli), &len);
      if (client_fd < 0) {
        if (!running_) break;
        if (errno == EINTR) continue;
        std::cerr << "[sim_server][tcp] accept failed: " << std::strerror(errno) << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        continue;
      }

      char ip[INET_ADDRSTRLEN]{};
      inet_ntop(AF_INET, &cli.sin_addr, ip, sizeof(ip));
      std::cout << "[sim_server][tcp] client connected: " << ip << ":" << ntohs(cli.sin_port) << "\n";

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

        demo::protocol::DetectionObject person{"person", conf(rng), 0.20 + (seq % 10) * 0.01, 0.20, 0.32, 0.46};
        demo::protocol::DetectionObject rod{"rod", conf(rng), 0.28 + ((seq + 2) % 10) * 0.01, 0.34, 0.30, 0.12};
        demo::protocol::DetectionObject boat{"boat", conf(rng), 0.60, 0.32, 0.30, 0.24};
        d.objects.push_back(person);
        if (seq % 2 == 0) d.objects.push_back(rod);
        d.objects.push_back(boat);

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
          std::cerr << "[sim_server][tcp] send failed / client disconnected\n";
          break;
        }

        ++seq;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      }

      ::close(client_fd);
    }

    if (listen_fd_ >= 0) {
      ::close(listen_fd_);
      listen_fd_ = -1;
    }
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "[sim_server][tcp] EXCEPTION in run(): " << e.what() << "\n";
  } catch (...) {
    std::cerr << "[sim_server][tcp] EXCEPTION in run(): unknown\n";
  }

  if (listen_fd_ >= 0) {
    ::close(listen_fd_);
    listen_fd_ = -1;
  }
  return 1;
}

void TcpSimServer::stop() {
  running_ = false;
  const int fd = listen_fd_.exchange(-1);
  if (fd >= 0) {
    ::shutdown(fd, SHUT_RDWR);
    ::close(fd);
  }
}

} // namespace demo::server
