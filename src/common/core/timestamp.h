#pragma once

#include <chrono>
#include <cstdint>

namespace demo::core {

inline std::int64_t now_ms() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

} // namespace demo::core
