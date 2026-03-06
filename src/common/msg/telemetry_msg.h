#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace demo::msg {

struct Header {
  std::string msg_type{"telemetry"};
  std::uint32_t version{1};
  std::uint64_t seq{0};
  std::int64_t sent_ts_ms{0};
};

struct DetectionObject {
  int label_id{-1};
  std::string label_name;
  double confidence{0.0};
  double cx{0.0};
  double cy{0.0};
  double w{0.0};
  double h{0.0};
};

struct Detection {
  std::string label{"multi"};
  double confidence{0.0};
  std::int64_t source_ts_ms{0};
  std::vector<DetectionObject> objects;
};

struct Gps {
  std::int64_t time_usec{0};
  int lat_e7{0};
  int lon_e7{0};
  int alt_mm{0};
  int eph_cm{0};
  int epv_cm{0};
  int vel_cms{0};
  int cog_cdeg{0};
  std::uint8_t fix_type{3};
  std::uint8_t satellites_visible{10};
};

struct Telemetry {
  Header header;
  Detection detection;
  Gps gps;
};

} // namespace demo::msg
