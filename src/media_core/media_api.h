#pragma once

#include <string>

namespace demo::media {

struct PullTestResult {
  bool probeOk{false};
  bool decodeOk{false};
  int probeCode{-1};
  int decodeCode{-1};
  std::string probeOutput;
  std::string decodeOutput;
};

class RtspPuller {
public:
  static PullTestResult testPull(const std::string& url, int seconds = 6);
};

} // namespace demo::media
