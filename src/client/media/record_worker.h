#pragma once

#include "core/types.h"
#include "database/database_service.h"

#include <QObject>
#include <QImage>
#include <QProcess>

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
  void start(const QString& outDir, const QString& rtspUrl);
  void stop();
  void enqueue(const RecordItem& item);

signals:
  void logMessage(const QString& msg);
  void playbackIndexed(const demo::client::PlaybackIndexRecord& rec);

private:
  bool running_{false};
  QString outDir_;
  QString rtspUrl_;
  int count_{0};
  int frameSeq_{0};
  QString currentVideoPath_;
  QString currentFramesDir_;
};

} // namespace demo::client

Q_DECLARE_METATYPE(demo::client::RecordItem)
