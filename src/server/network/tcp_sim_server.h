#pragma once

#include <atomic>
#include <cstdint>
#include <string>

namespace demo::server {

class TcpSimServer {
public:
  explicit TcpSimServer(std::uint16_t port);
  int run();
  void stop();

private:
  std::uint16_t port_;
  std::atomic<bool> running_{true};
};

} // namespace demo::server
