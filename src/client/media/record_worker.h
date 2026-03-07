#pragma once

#include "core/types.h"
#include "database/database_service.h"

#include <QObject>
#include <QImage>

namespace demo::client {

struct RecordItem {
  qint64 frameTsMs{0};
  QImage frameImage;
  bool frameValid{false};
  TelemetryPacket telemetry;
};

class RecordWorker : public QObject {
  Q_OBJECT
public:
  explicit RecordWorker(QObject* parent = nullptr);

public slots:
  void start(const QString& outDir);
  void stop();
  void enqueue(const RecordItem& item);

signals:
  void logMessage(const QString& msg);
  void playbackIndexed(const demo::client::PlaybackIndexRecord& rec);

private:
  bool running_{false};
  QString outDir_;
  int count_{0};
};

} // namespace demo::client

Q_DECLARE_METATYPE(demo::client::RecordItem)
