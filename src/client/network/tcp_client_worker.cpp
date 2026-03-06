#include "network/tcp_client_worker.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>

namespace demo::client {

TcpClientWorker::TcpClientWorker(QObject* parent) : QObject(parent) {
  socket_ = new QTcpSocket(this);
  connect(socket_, &QTcpSocket::readyRead, this, &TcpClientWorker::onReadyRead);
  connect(socket_, &QTcpSocket::connected, this, &TcpClientWorker::onConnected);
  connect(socket_, &QTcpSocket::disconnected, this, &TcpClientWorker::onDisconnected);
}

void TcpClientWorker::start(const QString& host, quint16 port) {
  emit logMessage(QString("TCP connecting to %1:%2").arg(host).arg(port));
  socket_->connectToHost(host, port);
}

void TcpClientWorker::stop() {
  socket_->disconnectFromHost();
}

void TcpClientWorker::onConnected() { emit logMessage("TCP connected"); }

void TcpClientWorker::onDisconnected() { emit logMessage("TCP disconnected"); }

void TcpClientWorker::onReadyRead() {
  buf_.append(socket_->readAll());
  while (true) {
    const int idx = buf_.indexOf('\n');
    if (idx < 0) break;
    const auto line = buf_.left(idx);
    buf_.remove(0, idx + 1);

    QJsonParseError err;
    const auto doc = QJsonDocument::fromJson(line, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
      emit logMessage("invalid json line");
      continue;
    }

    const auto root = doc.object();
    TelemetryPacket pkt;
    pkt.recvTsMs = QDateTime::currentMSecsSinceEpoch();
    pkt.sentTsMs = static_cast<qint64>(root.value("sent_ts_ms").toDouble());

    const auto det = root.value("detection").toObject();
    pkt.detection.label = det.value("label").toString();
    pkt.detection.confidence = det.value("confidence").toDouble();
    pkt.detection.sourceTsMs = static_cast<qint64>(det.value("source_ts_ms").toDouble());

    const auto gps = root.value("gps").toObject();
    pkt.gps.timeUsec = static_cast<qint64>(gps.value("time_usec").toDouble());
    pkt.gps.latE7 = gps.value("lat_e7").toInt();
    pkt.gps.lonE7 = gps.value("lon_e7").toInt();
    pkt.gps.altMm = gps.value("alt_mm").toInt();
    pkt.gps.velCms = gps.value("vel_cms").toInt();
    pkt.gps.cogCdeg = gps.value("cog_cdeg").toInt();
    pkt.gps.fixType = gps.value("fix_type").toInt();
    pkt.gps.satellitesVisible = gps.value("satellites_visible").toInt();

    emit telemetryReceived(pkt);
  }
}

} // namespace demo::client
