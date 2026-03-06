#pragma once

#include <QMetaType>
#include <QRectF>
#include <QString>
#include <QVector>

namespace demo::client {

struct DetectionObject {
  QString label;
  double confidence{0.0};
  QRectF bbox;
};

struct DetectionMsg {
  QString label;
  double confidence{0.0};
  qint64 sourceTsMs{0};
  QVector<DetectionObject> objects;
};

struct GpsMsg {
  qint64 timeUsec{0};
  int latE7{0};
  int lonE7{0};
  int altMm{0};
  int velCms{0};
  int cogCdeg{0};
  int fixType{0};
  int satellitesVisible{0};
};

struct TelemetryPacket {
  qint64 recvTsMs{0};
  qint64 sentTsMs{0};
  DetectionMsg detection;
  GpsMsg gps;
};

} // namespace demo::client

Q_DECLARE_METATYPE(demo::client::TelemetryPacket)
