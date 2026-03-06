#pragma once

#include "msg/telemetry_msg.h"

#include <sstream>
#include <string>

namespace demo::msg {

inline std::string to_json_line(const Telemetry& m) {
  std::ostringstream oss;
  oss << "{";
  oss << "\"type\":\"" << m.header.msg_type << "\",";
  oss << "\"version\":" << m.header.version << ",";
  oss << "\"seq\":" << m.header.seq << ",";
  oss << "\"sent_ts_ms\":" << m.header.sent_ts_ms << ",";

  oss << "\"detection\":{";
  oss << "\"label\":\"" << m.detection.label << "\",";
  oss << "\"confidence\":" << m.detection.confidence << ",";
  oss << "\"source_ts_ms\":" << m.detection.source_ts_ms << ",";
  oss << "\"objects\":[";

  for (size_t i = 0; i < m.detection.objects.size(); ++i) {
    const auto& o = m.detection.objects[i];
    if (i) oss << ",";
    oss << "{";
    oss << "\"label_id\":" << o.label_id << ",";
    oss << "\"label\":\"" << o.label_name << "\",";
    oss << "\"confidence\":" << o.confidence << ",";
    oss << "\"cx\":" << o.cx << ",";
    oss << "\"cy\":" << o.cy << ",";
    oss << "\"w\":" << o.w << ",";
    oss << "\"h\":" << o.h << ",";
    oss << "\"bbox\":[" << o.cx << "," << o.cy << "," << o.w << "," << o.h << "]";
    oss << "}";
  }

  oss << "]},";

  oss << "\"gps\":{";
  oss << "\"time_usec\":" << m.gps.time_usec << ",";
  oss << "\"lat_e7\":" << m.gps.lat_e7 << ",";
  oss << "\"lon_e7\":" << m.gps.lon_e7 << ",";
  oss << "\"alt_mm\":" << m.gps.alt_mm << ",";
  oss << "\"satellites_visible\":" << static_cast<int>(m.gps.satellites_visible) << "}";

  oss << "}\n";
  return oss.str();
}

} // namespace demo::msg
