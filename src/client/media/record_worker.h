#pragma once

#include "core/types.h"

#include <QObject>
#include <QVideoFrame>

namespace demo::client {

struct RecordItem {
  qint64 frameTsMs{0};
  QVideoFrame frame;
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

private:
  bool running_{false};
  QString outDir_;
  int count_{0};
};

} // namespace demo::client

Q_DECLARE_METATYPE(demo::client::RecordItem)
