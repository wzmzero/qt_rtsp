#include "media_api.h"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
  std::string url = "rtsp://127.0.0.1:8554/live";
  int sec = 6;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if ((arg == "-u" || arg == "--url") && i + 1 < argc) {
      url = argv[++i];
    } else if ((arg == "-t" || arg == "--seconds") && i + 1 < argc) {
      sec = std::stoi(argv[++i]);
    } else if (arg == "-h" || arg == "--help") {
      std::cout << "Usage: test_rtsp_pull_api [--url <rtsp_url>] [--seconds <n>]\n";
      return 0;
    }
  }

  auto r = demo::media::RtspPuller::testPull(url, sec);
  std::cout << "[PROBE] code=" << r.probeCode << (r.probeOk ? " PASS" : " FAIL") << "\n";
  std::cout << "[DECODE] code=" << r.decodeCode << (r.decodeOk ? " PASS" : " FAIL") << "\n";
  if (!r.probeOk) std::cout << r.probeOutput << "\n";
  if (!r.decodeOk) std::cout << r.decodeOutput << "\n";

  const bool ok = r.probeOk && r.decodeOk;
  std::cout << "RESULT: " << (ok ? "PASS" : "FAIL") << "\n";
  return ok ? 0 : 1;
}
