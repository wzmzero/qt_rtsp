#include "media_api.h"

#include <array>
#include <cstdio>
#include <sstream>
#include <string>
#include <sys/wait.h>

namespace demo::media {
namespace {

struct CmdResult {
  int code{127};
  std::string output;
};

CmdResult runCmd(const std::string& cmd) {
  CmdResult r;
  std::array<char, 4096> buf{};
  std::string out;

  FILE* pipe = popen((cmd + " 2>&1").c_str(), "r");
  if (!pipe) {
    r.output = "popen failed";
    return r;
  }

  while (fgets(buf.data(), static_cast<int>(buf.size()), pipe)) {
    out += buf.data();
  }

  const int status = pclose(pipe);
  if (WIFEXITED(status)) {
    r.code = WEXITSTATUS(status);
  } else {
    r.code = 128;
  }
  r.output = out;
  return r;
}

std::string shQuote(const std::string& s) {
  std::ostringstream oss;
  oss << '\'';
  for (char c : s) {
    if (c == '\'') oss << "'\\''";
    else oss << c;
  }
  oss << '\'';
  return oss.str();
}

} // namespace

PullTestResult RtspPuller::testPull(const std::string& url, int seconds) {
  PullTestResult r;
  if (seconds <= 0) seconds = 6;

  const std::string qurl = shQuote(url);

  const std::string probeCmd =
      "ffprobe -v error -rtsp_transport tcp "
      "-select_streams v:0 -show_entries stream=codec_name,width,height "
      "-of default=nw=1:nk=1 " + qurl;

  const auto probe = runCmd(probeCmd);
  r.probeCode = probe.code;
  r.probeOutput = probe.output;
  r.probeOk = (probe.code == 0 && probe.output.find("h264") != std::string::npos);

  std::ostringstream dcmd;
  dcmd << "ffmpeg -v warning -rtsp_transport tcp "
       << "-fflags +genpts+discardcorrupt -flags low_delay "
       << "-hwaccel none -t " << seconds << " -i " << qurl
       << " -an -sn -dn -f null -";

  const auto dec = runCmd(dcmd.str());
  r.decodeCode = dec.code;
  r.decodeOutput = dec.output;
  r.decodeOk = (dec.code == 0 && dec.output.find("Error") == std::string::npos &&
                dec.output.find("error") == std::string::npos);

  return r;
}

} // namespace demo::media
