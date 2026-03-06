#include "app/main_window.h"
#include "core/app_config.h"
#include "core/types.h"
#include "database/database_service.h"
#include "media/record_worker.h"

#include <QApplication>
#include <QByteArray>

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
  // 强制使用 FFmpeg 后端并放开 RTSP 协议白名单（解决部分环境下 RTSP 拉流失败）
  qputenv("QT_MEDIA_BACKEND", QByteArray("ffmpeg"));
  qputenv("QT_FFMPEG_PROTOCOL_WHITELIST", QByteArray("file,udp,rtp,tcp,rtsp"));
  // 禁用硬件解码探测，避免 VAAPI/CUDA 环境导致拉流失败
  qputenv("QT_FFMPEG_DECODING_HW_DEVICE_TYPES", QByteArray(""));

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
