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

void RecordWorker::start(const QString& outDir, const QString& rtspUrl) {
  running_ = true;
  outDir_ = outDir;
  rtspUrl_ = rtspUrl;
  count_ = 0;
  frameSeq_ = 0;
  QDir().mkpath(outDir_);

  const QDateTime now = QDateTime::currentDateTime();
  const QString dayDirName = now.toString("yyyyMMdd");
  const QString dayDir = QDir(outDir_).filePath(dayDirName);
  QDir().mkpath(dayDir);

  // 以30分钟为粒度分桶，避免文件粒度过小
  const int bucketMin = (now.time().minute() / 30) * 30;
  const QDateTime bucketStart(now.date(), QTime(now.time().hour(), bucketMin, 0));
  const QString bucketTag = bucketStart.toString("yyyyMMdd_HHmm");

  QString baseName = QString("record_%1").arg(bucketTag);
  QString videoPath = QDir(dayDir).filePath(baseName + ".mp4");
  int part = 1;
  while (QFile::exists(videoPath)) {
    ++part;
    videoPath = QDir(dayDir).filePath(QString("%1_part%2.mp4").arg(baseName).arg(part));
  }
  currentVideoPath_ = videoPath;

  QString framesName = QString("frames_%1").arg(bucketTag);
  QString framesDir = QDir(dayDir).filePath(framesName);
  int fpart = 1;
  while (QFile::exists(framesDir)) {
    ++fpart;
    framesDir = QDir(dayDir).filePath(QString("%1_part%2").arg(framesName).arg(fpart));
  }
  currentFramesDir_ = framesDir;
  QDir().mkpath(currentFramesDir_);

  emit logMessage(QString("Record worker started: %1, file=%2").arg(dayDir, currentVideoPath_));
}

void RecordWorker::stop() {
  running_ = false;

  if (!currentFramesDir_.isEmpty() && frameSeq_ > 1) {
    QStringList args;
    args << "-y"
         << "-framerate" << "12"
         << "-i" << QDir(currentFramesDir_).filePath("frame_%06d.jpg")
         << "-c:v" << "libx264"
         << "-preset" << "veryfast"
         << "-pix_fmt" << "yuv420p"
         << currentVideoPath_;
    const int code = QProcess::execute("ffmpeg", args);
    emit logMessage(QString("Record compose video exit=%1 file=%2").arg(code).arg(currentVideoPath_));
  }

  emit logMessage(QString("Record worker stopped, file=%1").arg(currentVideoPath_));
}

void RecordWorker::enqueue(const RecordItem& item) {
  if (!running_) return;

  ++count_;

  if (!item.frameImage.isNull() && (count_ % 2 == 0)) { // 约 12fps
    ++frameSeq_;
    const QString imgPath = QDir(currentFramesDir_).filePath(QString("frame_%1.jpg").arg(frameSeq_, 6, 10, QChar('0')));
    item.frameImage.save(imgPath, "JPG", 88);
  }

  if (count_ % 15 != 0) return; // 元数据抽样写入

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
}


} // namespace demo::client
