#include "app/main_window.h"
#include "core/app_config.h"
#include "core/types.h"
#include "media/record_worker.h"

#include <QApplication>

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);

  qRegisterMetaType<demo::client::TelemetryPacket>("demo::client::TelemetryPacket");
  qRegisterMetaType<demo::client::RecordItem>("demo::client::RecordItem");
  qRegisterMetaType<demo::client::AppConfig>("demo::client::AppConfig");
  qRegisterMetaType<QVideoFrame>("QVideoFrame");

  MainWindow w;
  w.show();

  return app.exec();
}
