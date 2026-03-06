#include "media/record_worker.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

namespace demo::client {

RecordWorker::RecordWorker(QObject* parent) : QObject(parent) {}

void RecordWorker::start(const QString& outDir) {
  running_ = true;
  outDir_ = outDir;
  count_ = 0;
  QDir().mkpath(outDir_);
  emit logMessage(QString("Record worker started: %1").arg(outDir_));
}

void RecordWorker::stop() {
  running_ = false;
  emit logMessage("Record worker stopped");
}

void RecordWorker::enqueue(const RecordItem& item) {
  if (!running_) return;

  ++count_;
  if (count_ % 15 != 0) return; // 降低磁盘压力，抽样写入

  QJsonObject root;
  root["wall_ts_ms"] = QDateTime::currentMSecsSinceEpoch();
  root["frame_ts_ms"] = item.frameTsMs;
  root["frame_valid"] = item.frame.isValid();
  root["latency_ms"] = item.telemetry.sentTsMs > 0 ? item.frameTsMs - item.telemetry.sentTsMs : 0;

  QJsonObject det;
  det["label"] = item.telemetry.detection.label;
  det["confidence"] = item.telemetry.detection.confidence;
  det["source_ts_ms"] = item.telemetry.detection.sourceTsMs;
  root["detection"] = det;

  QJsonObject gps;
  gps["time_usec"] = QString::number(item.telemetry.gps.timeUsec);
  gps["lat_e7"] = item.telemetry.gps.latE7;
  gps["lon_e7"] = item.telemetry.gps.lonE7;
  gps["alt_mm"] = item.telemetry.gps.altMm;
  gps["vel_cms"] = item.telemetry.gps.velCms;
  gps["cog_cdeg"] = item.telemetry.gps.cogCdeg;
  gps["fix_type"] = item.telemetry.gps.fixType;
  gps["satellites_visible"] = item.telemetry.gps.satellitesVisible;
  root["gps"] = gps;

  QFile f(outDir_ + "/record_meta.jsonl");
  if (!f.open(QIODevice::Append | QIODevice::Text)) {
    emit logMessage("record worker: open record_meta.jsonl failed");
    return;
  }
  QTextStream ts(&f);
  ts << QJsonDocument(root).toJson(QJsonDocument::Compact) << '\n';
}

} // namespace demo::client
