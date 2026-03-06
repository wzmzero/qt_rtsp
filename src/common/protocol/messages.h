#pragma once

#include "telemetry.pb.h"

#include <cstdint>
#include <string>
#include <vector>

namespace demo::protocol {

struct DetectionObject {
  int label_id{-1};
  std::string label;
  double confidence{0.0};
  double cx{0.0};
  double cy{0.0};
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

inline std::string to_protobuf_payload(const Detection& d, const Gps& g, std::int64_t sent_ts_ms,
                                       std::uint64_t seq = 0, std::uint32_t version = 1) {
  demo::msg::Telemetry msg;
  msg.set_type("telemetry");
  msg.set_version(version);
  msg.set_seq(seq);
  msg.set_sent_ts_ms(sent_ts_ms);

  auto* det = msg.mutable_detection();
  det->set_label(d.label);
  det->set_confidence(d.confidence);
  det->set_source_ts_ms(d.source_ts_ms);
  for (const auto& o : d.objects) {
    auto* out = det->add_objects();
    out->set_label_id(o.label_id);
    out->set_label(o.label);
    out->set_confidence(o.confidence);
    out->set_cx(o.cx);
    out->set_cy(o.cy);
    out->set_w(o.w);
    out->set_h(o.h);
  }

  auto* gps = msg.mutable_gps();
  gps->set_time_usec(g.time_usec);
  gps->set_lat_e7(g.lat_e7);
  gps->set_lon_e7(g.lon_e7);
  gps->set_alt_mm(g.alt_mm);
  gps->set_eph_cm(g.eph_cm);
  gps->set_epv_cm(g.epv_cm);
  gps->set_vel_cms(g.vel_cms);
  gps->set_cog_cdeg(g.cog_cdeg);
  gps->set_fix_type(g.fix_type);
  gps->set_satellites_visible(g.satellites_visible);

  std::string payload;
  msg.SerializeToString(&payload);
  return payload;
}

inline std::string to_protobuf_frame(const Detection& d, const Gps& g, std::int64_t sent_ts_ms,
                                     std::uint64_t seq = 0, std::uint32_t version = 1) {
  const std::string payload = to_protobuf_payload(d, g, sent_ts_ms, seq, version);
  const std::uint32_t n = static_cast<std::uint32_t>(payload.size());
  std::string frame;
  frame.resize(4 + payload.size());
  frame[0] = static_cast<char>((n >> 24) & 0xFF);
  frame[1] = static_cast<char>((n >> 16) & 0xFF);
  frame[2] = static_cast<char>((n >> 8) & 0xFF);
  frame[3] = static_cast<char>(n & 0xFF);
  frame.replace(4, payload.size(), payload);
  return frame;
}

} // namespace demo::protocol
