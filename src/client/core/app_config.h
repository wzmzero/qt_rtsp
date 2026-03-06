#pragma once
#include <QMetaType>
#include <QtGlobal>

#include <QByteArray>
#include <QString>

namespace demo::client {

struct AppConfig {
  QString rtspUrl{"rtsp://127.0.0.1:8554/live"};
  QString tcpHost{"127.0.0.1"};
  quint16 tcpPort{9000};
  int reconnectIntervalMs{3000};
  QString recordDir{"./recordings"};
  bool recordEnabled{true};
  QString theme{"dark"};
  QByteArray windowGeometry;
  QByteArray windowState;
};

} // namespace demo::client

Q_DECLARE_METATYPE(demo::client::AppConfig)
