#include "network/tcp_sim_server.h"

#include "core/timestamp.h"
#include "protocol/messages.h"

#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <exception>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <random>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <algorithm>
#include <vector>
#include <sstream>
#include <map>
#include <fstream>
#include <unistd.h>

namespace demo::server {


namespace {
std::map<int, std::string> loadClassMap(const std::string& path) {
  std::map<int, std::string> mp{{0, "person"}, {1, "rod"}};
  if (path.empty()) return mp;

  std::ifstream in(path);
  if (!in.is_open()) return mp;

  std::string line;
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == '#') continue;
    std::replace(line.begin(), line.end(), ':', ' ');
    std::istringstream iss(line);
    int id = -1;
    std::string name;
    if (iss >> id >> name) mp[id] = name;
  }
  return mp;
}

std::vector<demo::protocol::DetectionObject> loadYoloObjects(const std::string& txtPath,
                                                            const std::map<int, std::string>& classMap,
                                                            bool& ok) {
  ok = false;
  std::vector<demo::protocol::DetectionObject> out;
  if (txtPath.empty()) return out;

  std::ifstream in(txtPath);
  if (!in.is_open()) return out;

  std::string line;
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == '#') continue;
    std::istringstream iss(line);
    int cls = -1;
    double cx = 0, cy = 0, w = 0, h = 0, conf = 0;
    if (!(iss >> cls >> cx >> cy >> w >> h >> conf)) continue;

    demo::protocol::DetectionObject o;
    auto it = classMap.find(cls);
    o.label_id = cls;
    o.label = (it == classMap.end()) ? ("cls_" + std::to_string(cls)) : it->second;
    o.confidence = conf;
    o.cx = cx;
    o.cy = cy;
    o.w = w;
    o.h = h;
    out.push_back(o);
  }

  ok = !out.empty();
  return out;
}
} // namespace

TcpSimServer::TcpSimServer(std::uint16_t port) : port_(port) {}

TcpSimServer::TcpSimServer(std::uint16_t port, const SimConfig& cfg) : port_(port), cfg_(cfg) {}

int TcpSimServer::run() {
  try {
    running_ = true;

    int listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
      std::cerr << "[sim_server][tcp] socket failed: " << std::strerror(errno) << "\n";
      return 1;
    }
    listen_fd_ = listen_fd;

    const int fd_flags = ::fcntl(listen_fd, F_GETFD, 0);
    if (fd_flags >= 0) {
      (void)::fcntl(listen_fd, F_SETFD, fd_flags | FD_CLOEXEC);
    }

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

      const int client_flags = ::fcntl(client_fd, F_GETFD, 0);
      if (client_flags >= 0) {
        (void)::fcntl(client_fd, F_SETFD, client_flags | FD_CLOEXEC);
      }

      std::mt19937 rng{std::random_device{}()};
      std::uniform_int_distribution<int> shift(-30, 30);

      const auto classMap = loadClassMap(cfg_.class_map_path);

      int seq = 0;
      while (running_) {
        const auto now_ms = demo::core::now_ms();

        demo::protocol::Detection d;
        d.label = "person";
        d.confidence = 0.95;
        d.source_ts_ms = now_ms - 15;

        bool yoloOk = false;
        auto yoloObjects = loadYoloObjects(cfg_.yolo_txt_path, classMap, yoloOk);

        if (yoloOk) {
          d.label = "multi";
          d.confidence = 1.0;
          d.objects = std::move(yoloObjects);
        } else {
          // 仅 person + rod；若给定固定参数则严格按固定参数发送
          const bool fixed = cfg_.fixed;
          const double personCx = fixed ? cfg_.person_cx : (0.36 + (seq % 6) * 0.015);
          const double personCy = fixed ? cfg_.person_cy : 0.38;
          const double personW = cfg_.person_w;
          const double personH = cfg_.person_h;
          const double rodCx = fixed ? cfg_.rod_cx : (personCx + 0.02);
          const double rodCy = fixed ? cfg_.rod_cy : (personCy + 0.02);

          const double personConf = cfg_.person_conf;
          const double rodConf = cfg_.rod_conf;
          demo::protocol::DetectionObject person;
          person.label_id = 0;
          person.label = "person";
          person.confidence = personConf;
          person.cx = personCx;
          person.cy = personCy;
          person.w = personW;
          person.h = personH;

          demo::protocol::DetectionObject rod;
          rod.label_id = 1;
          rod.label = "rod";
          rod.confidence = rodConf;
          rod.cx = rodCx;
          rod.cy = rodCy;
          rod.w = cfg_.rod_w;
          rod.h = cfg_.rod_h;

          d.objects.push_back(person);
          d.objects.push_back(rod);
        }
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

        const auto frame = demo::protocol::to_protobuf_frame(d, g, now_ms);
        const auto ret = ::send(client_fd, frame.data(), frame.size(), MSG_NOSIGNAL);
        if (ret <= 0) {
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
