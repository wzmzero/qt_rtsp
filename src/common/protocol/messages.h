#pragma once

#include "msg/json_codec.h"
#include "msg/telemetry_msg.h"

#include <cstdint>
#include <string>
#include <vector>

namespace demo::protocol {

// Compatibility aliases: existing code can keep using demo::protocol::* while
// underlying wire model is standardized in demo::msg::*.
using DetectionObject = demo::msg::DetectionObject;
using Detection = demo::msg::Detection;
using Gps = demo::msg::Gps;

inline std::string to_json_line(const Detection& d, const Gps& g, std::int64_t sent_ts_ms,
                                std::uint64_t seq = 0, std::uint32_t version = 1) {
  demo::msg::Telemetry msg;
  msg.header.msg_type = "telemetry";
  msg.header.version = version;
  msg.header.seq = seq;
  msg.header.sent_ts_ms = sent_ts_ms;
  msg.detection = d;
  msg.gps = g;
  return demo::msg::to_json_line(msg);
}

} // namespace demo::protocol
