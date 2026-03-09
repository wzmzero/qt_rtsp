#include "app/main_window.h"
#include "core/app_config.h"
#include "core/types.h"
#include "database/database_service.h"
#include "media/record_worker.h"

#include <QApplication>
#include <QByteArray>
#include <QDir>

#include <dlfcn.h>

#include <cstdio>
#include <csignal>
#include <execinfo.h>
#include <unistd.h>
#include <iostream>
#include <string>

namespace {
void redirectStderrToLogFile() {
  const QString logsDir = QDir::current().filePath("./logs");
  QDir().mkpath(logsDir);
  const QString stderrLog = QDir(logsDir).filePath("qt_ffmpeg_stderr.log");
  (void)freopen(stderrLog.toLocal8Bit().constData(), "a", stderr);
}


void installCrashHandlers() {
  auto handler = +[](int sig) {
    const char* name = (sig == SIGFPE) ? "SIGFPE" : (sig == SIGSEGV) ? "SIGSEGV" : (sig == SIGABRT) ? "SIGABRT" : "SIGNAL";
    const QString logsDir = QDir::current().filePath("./logs");
    QDir().mkpath(logsDir);
    const QString crashLog = QDir(logsDir).filePath("qt_client_crash.log");
    FILE* f = fopen(crashLog.toLocal8Bit().constData(), "a");
    if (f) {
      fprintf(f, "\n=== crash caught: %s (%d) ===\n", name, sig);
      void* bt[64];
      int n = backtrace(bt, 64);
      backtrace_symbols_fd(bt, n, fileno(f));
      fclose(f);
    }
    _exit(128 + sig);
  };
  std::signal(SIGFPE, handler);
  std::signal(SIGSEGV, handler);
  std::signal(SIGABRT, handler);
}

void silenceFfmpegAvLog() {
  using AvLogSetLevelFn = void (*)(int);
  constexpr int AV_LOG_QUIET = -8;
  const char* libs[] = {"libavutil.so", "libavutil.so.58", "libavutil.so.57"};
  for (const char* lib : libs) {
    void* h = dlopen(lib, RTLD_NOW | RTLD_GLOBAL);
    if (!h) continue;
    auto fn = reinterpret_cast<AvLogSetLevelFn>(dlsym(h, "av_log_set_level"));
    if (fn) {
      fn(AV_LOG_QUIET);
      return;
    }
  }
}
} // namespace

int main(int argc, char* argv[]) {
  // Linux 下优先 gstreamer（更稳），未安装时可手动 QT_MEDIA_BACKEND=ffmpeg 覆盖
  if (!qEnvironmentVariableIsSet("QT_MEDIA_BACKEND")) {
#ifdef __linux__
    qputenv("QT_MEDIA_BACKEND", QByteArray("gstreamer"));
#else
    qputenv("QT_MEDIA_BACKEND", QByteArray("ffmpeg"));
#endif
  }
  qputenv("QT_FFMPEG_PROTOCOL_WHITELIST", QByteArray("file,udp,rtp,tcp,rtsp"));
  // 强制软解码，避免 VAAPI/CUDA -> 纹理导入失败（failed to get textures for frame）
  qputenv("QT_FFMPEG_DECODING_HW_DEVICE_TYPES", QByteArray("none"));
  // 禁用硬件帧纹理转换路径，规避驱动/上下文不兼容
  qputenv("QT_DISABLE_HW_TEXTURES_CONVERSION", QByteArray("1"));
  // 强制软件 OpenGL，进一步规避 "failed to get textures for frame" 类驱动问题
  qputenv("QT_OPENGL", QByteArray("software"));
  qputenv("LIBGL_ALWAYS_SOFTWARE", QByteArray("1"));
  // 关闭 FFmpeg 探测调试输出，避免终端刷屏（如 Checking HW acceleration...）
  qputenv("QT_FFMPEG_DEBUG", QByteArray("0"));
  qputenv("QT_LOGGING_RULES", QByteArray("qt.multimedia.ffmpeg.*=false;qt.multimedia.ffmpeg=false"));
  installCrashHandlers();

  if (!qEnvironmentVariableIsSet("QT_CLIENT_STDERR_CONSOLE")) {
    redirectStderrToLogFile();
  }
  silenceFfmpegAvLog();

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      std::cout << "qt_client 用法:\n"
                << "  qt_client\n\n"
                << "说明:\n"
                << "  图形客户端程序，参数由界面和配置文件控制。\n\n"
                << "示例:\n"
                << "  ./qt_client\n";
      return 0;
    }

  }

  QApplication app(argc, argv);

  qRegisterMetaType<demo::client::TelemetryPacket>("demo::client::TelemetryPacket");
  qRegisterMetaType<demo::client::RecordItem>("demo::client::RecordItem");
  qRegisterMetaType<demo::client::AppConfig>("demo::client::AppConfig");
  qRegisterMetaType<QVideoFrame>("QVideoFrame");
  qRegisterMetaType<demo::client::PlaybackIndexRecord>("demo::client::PlaybackIndexRecord");

  MainWindow w;
  w.show();

  return app.exec();
}
