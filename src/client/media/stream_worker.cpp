#include "media/stream_worker.h"

#include <QDateTime>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QTimer>
#include <QUrlQuery>

namespace demo::client {

StreamWorker::StreamWorker(QObject* parent) : QObject(parent) {
  player_ = new QMediaPlayer(this);
  sink_ = new QVideoSink(this);
  watchdog_ = new QTimer(this);
  watchdog_->setInterval(3000);
  player_->setVideoSink(sink_);
  connect(sink_, &QVideoSink::videoFrameChanged, this, &StreamWorker::onFrameChanged);
  connect(player_, &QMediaPlayer::errorOccurred, this, &StreamWorker::onPlayerError);
  connect(watchdog_, &QTimer::timeout, this, &StreamWorker::checkStreamAlive);
}

void StreamWorker::start(const QUrl& rtspUrl) {
  baseUrl_ = rtspUrl;
  triedTcpParam_ = false;
  lastFrameTsMs_ = 0;
  emit logMessage(QString("Start RTSP pull: %1").arg(rtspUrl.toString()));
  player_->setSource(rtspUrl);
  player_->play();
  watchdog_->start();
}

void StreamWorker::stop() {
  if (watchdog_) watchdog_->stop();
  player_->stop();
  emit logMessage("RTSP stopped");
}

void StreamWorker::onFrameChanged(const QVideoFrame& frame) {
  if (!frame.isValid()) return;
  lastFrameTsMs_ = QDateTime::currentMSecsSinceEpoch();
  emit frameArrived(frame, lastFrameTsMs_);
}

void StreamWorker::onPlayerError() {
  emit logMessage(QString("RTSP player error: %1").arg(player_->errorString()));
}

void StreamWorker::checkStreamAlive() {
  const qint64 now = QDateTime::currentMSecsSinceEpoch();
  if (lastFrameTsMs_ > 0 && now - lastFrameTsMs_ < 3500) return;

  if (!triedTcpParam_ && baseUrl_.scheme().startsWith("rtsp")) {
    triedTcpParam_ = true;
    QUrl tcpUrl = baseUrl_;
    QUrlQuery q(tcpUrl);
    q.addQueryItem("rtsp_transport", "tcp");
    tcpUrl.setQuery(q);
    emit logMessage(QString("No frame yet, retry with TCP transport param: %1").arg(tcpUrl.toString()));
    player_->stop();
    player_->setSource(tcpUrl);
    player_->play();
    return;
  }

  emit logMessage("RTSP no frame timeout, reconnecting...");
  player_->stop();
  player_->setSource(baseUrl_);
  player_->play();
}

} // namespace demo::client
