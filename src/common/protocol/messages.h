#pragma once

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace demo::protocol {

struct DetectionObject {
  std::string label;
  double confidence{0.0};
  double x{0.0};
  double y{0.0};
  double w{0.0};
  double h{0.0};
};

struct Detection {
  std::string label;
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

inline std::string to_json_line(const Detection& d, const Gps& g, std::int64_t sent_ts_ms) {
  std::ostringstream oss;
  oss << "{"
      << "\"type\":\"telemetry\"," 
      << "\"sent_ts_ms\":" << sent_ts_ms << ","
      << "\"detection\":{"
      << "\"label\":\"" << d.label << "\"," 
      << "\"confidence\":" << d.confidence << ","
      << "\"source_ts_ms\":" << d.source_ts_ms << ","
      << "\"objects\":[";
  for (size_t i = 0; i < d.objects.size(); ++i) {
    const auto& o = d.objects[i];
    if (i) oss << ",";
    oss << "{" << "\"label\":\"" << o.label << "\"," << "\"confidence\":" << o.confidence << ","
        << "\"cx\":" << o.x << "," << "\"cy\":" << o.y << ","
        << "\"w\":" << o.w << "," << "\"h\":" << o.h << ","
        << "\"bbox\":[" << o.x << "," << o.y << "," << o.w << "," << o.h << "]" << "}";
  }
  oss << "]},"
      << "\"gps\":{"
      << "\"time_usec\":" << g.time_usec << ","
      << "\"lat_e7\":" << g.lat_e7 << ","
      << "\"lon_e7\":" << g.lon_e7 << ","
      << "\"alt_mm\":" << g.alt_mm << ","
      << "\"eph_cm\":" << g.eph_cm << ","
      << "\"epv_cm\":" << g.epv_cm << ","
      << "\"vel_cms\":" << g.vel_cms << ","
      << "\"cog_cdeg\":" << g.cog_cdeg << ","
      << "\"fix_type\":" << static_cast<int>(g.fix_type) << ","
      << "\"satellites_visible\":" << static_cast<int>(g.satellites_visible) << "}"
      << "}";
  oss << "\n";
  return oss.str();
}

} // namespace demo::protocol
