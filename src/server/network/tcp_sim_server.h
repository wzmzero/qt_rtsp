#pragma once

#include <atomic>
#include <cstdint>

namespace demo::server {

class TcpSimServer {
public:
  struct SimConfig {
    bool fixed = false;
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
  };

  explicit TcpSimServer(std::uint16_t port);
  TcpSimServer(std::uint16_t port, const SimConfig& cfg);
  int run();
  void stop();

private:
  std::uint16_t port_;
  SimConfig cfg_;
  std::atomic<bool> running_{true};
  std::atomic<int> listen_fd_{-1};
};

} // namespace demo::server
