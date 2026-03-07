#include "media/record_worker.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QImage>

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
  root["frame_valid"] = item.frameValid;
  root["latency_ms"] = item.telemetry.sentTsMs > 0 ? item.frameTsMs - item.telemetry.sentTsMs : 0;

  QJsonObject det;
  det["label"] = item.telemetry.detection.label;
  det["confidence"] = item.telemetry.detection.confidence;
  det["source_ts_ms"] = item.telemetry.detection.sourceTsMs;
  QJsonArray objs;
  for (const auto& obj : item.telemetry.detection.objects) {
    QJsonObject jo;
    jo["label"] = obj.label;
    jo["confidence"] = obj.confidence;
    jo["bbox"] = QJsonArray{obj.bbox.x(), obj.bbox.y(), obj.bbox.width(), obj.bbox.height()};
    objs.push_back(jo);
  }
  det["objects"] = objs;
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

  const qint64 frameTs = item.frameTsMs > 0 ? item.frameTsMs : QDateTime::currentMSecsSinceEpoch();
  const QString imageAbs = outDir_ + QString("/frame_%1.jpg").arg(frameTs);
  bool imageSaved = false;
  if (!item.frameImage.isNull()) {
    imageSaved = item.frameImage.save(imageAbs, "JPG", 90);
  }

  demo::client::PlaybackIndexRecord rec;
  rec.frameTsMs = frameTs;
  rec.wallTsMs = root["wall_ts_ms"].toInteger();
  const QString p = imageSaved ? imageAbs : (outDir_ + "/record_meta.jsonl");
  const QString rel = QDir::current().relativeFilePath(p);
  rec.metaPath = rel.startsWith("..") ? p : QString("./%1").arg(rel);
  rec.latencyMs = root["latency_ms"].toInteger();
  rec.label = item.telemetry.detection.label;
  rec.confidence = item.telemetry.detection.confidence;
  rec.latDeg = static_cast<double>(item.telemetry.gps.latE7) / 10000000.0;
  rec.lonDeg = static_cast<double>(item.telemetry.gps.lonE7) / 10000000.0;
  emit playbackIndexed(rec);
}


} // namespace demo::client
