#include "media/stream_worker.h"

#include <QDateTime>
#include <QMediaPlayer>
#include <QVideoSink>

namespace demo::client {

StreamWorker::StreamWorker(QObject* parent) : QObject(parent) {
  player_ = new QMediaPlayer(this);
  sink_ = new QVideoSink(this);
  player_->setVideoSink(sink_);
  connect(sink_, &QVideoSink::videoFrameChanged, this, &StreamWorker::onFrameChanged);
}

void StreamWorker::start(const QUrl& rtspUrl) {
  emit logMessage(QString("Start RTSP pull: %1").arg(rtspUrl.toString()));
  player_->setSource(rtspUrl);
  player_->play();
}

void StreamWorker::stop() {
  player_->stop();
  emit logMessage("RTSP stopped");
}

void StreamWorker::onFrameChanged(const QVideoFrame& frame) {
  if (!frame.isValid()) return;
  emit frameArrived(frame, QDateTime::currentMSecsSinceEpoch());
}

} // namespace demo::client
