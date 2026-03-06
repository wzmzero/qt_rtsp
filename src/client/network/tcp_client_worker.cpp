#include "network/tcp_client_worker.h"

#include <QDateTime>
#include "message.pb.h"

namespace demo::client {

TcpClientWorker::TcpClientWorker(QObject* parent) : QObject(parent) {
  socket_ = new QTcpSocket(this);
  reconnectTimer_.setSingleShot(true);

  connect(socket_, &QTcpSocket::readyRead, this, &TcpClientWorker::onReadyRead);
  connect(socket_, &QTcpSocket::connected, this, &TcpClientWorker::onConnected);
  connect(socket_, &QTcpSocket::disconnected, this, &TcpClientWorker::onDisconnected);
  connect(&reconnectTimer_, &QTimer::timeout, this, &TcpClientWorker::reconnect);
}

void TcpClientWorker::start(const QString& host, quint16 port, int reconnectIntervalMs) {
  host_ = host;
  port_ = port;
  reconnectIntervalMs_ = reconnectIntervalMs;
  running_ = true;
  reconnect();
}

void TcpClientWorker::reconnect() {
  if (!running_) return;
  emit connectionStateChanged("connecting");
  emit logMessage(QString("TCP connecting to %1:%2").arg(host_).arg(port_));
  socket_->abort();
  socket_->connectToHost(host_, port_);
}

void TcpClientWorker::stop() {
  running_ = false;
  reconnectTimer_.stop();
  socket_->disconnectFromHost();
}

void TcpClientWorker::onConnected() {
  emit connectionStateChanged("connected");
  emit logMessage("TCP connected");
}

void TcpClientWorker::onDisconnected() {
  emit connectionStateChanged("disconnected");
  emit logMessage("TCP disconnected");
  if (running_) reconnectTimer_.start(reconnectIntervalMs_);
}

void TcpClientWorker::onReadyRead() {
  buf_.append(socket_->readAll());

  while (true) {
    if (buf_.size() < 4) break;

    const quint32 len = (static_cast<quint8>(buf_.at(0)) << 24) |
                        (static_cast<quint8>(buf_.at(1)) << 16) |
                        (static_cast<quint8>(buf_.at(2)) << 8) |
                        static_cast<quint8>(buf_.at(3));

    if (len == 0 || len > 10 * 1024 * 1024) {
      emit logMessage("invalid protobuf frame length");
      buf_.clear();
      break;
    }

    if (buf_.size() < 4 + static_cast<int>(len)) break;

    const QByteArray payload = buf_.mid(4, static_cast<int>(len));
    buf_.remove(0, 4 + static_cast<int>(len));

    demo::msg::Telemetry msg;
    if (!msg.ParseFromArray(payload.constData(), payload.size())) {
      emit logMessage("invalid protobuf payload");
      continue;
    }

    TelemetryPacket pkt;
    pkt.recvTsMs = QDateTime::currentMSecsSinceEpoch();
    pkt.sentTsMs = msg.sent_ts_ms();
    pkt.rawJsonLine = QString::fromUtf8(payload.toHex(' '));

    pkt.detection.label = "multi";
    pkt.detection.confidence = 0.0;
    pkt.detection.sourceTsMs = msg.source_ts_ms();

    for (const auto& o : msg.detections()) {
      demo::client::DetectionObject obj;
      obj.label = QString::fromStdString(o.label());
      obj.confidence = o.confidence();
      obj.bbox = QRectF(o.cx(), o.cy(), o.w(), o.h());
      pkt.detection.objects.push_back(obj);
    }

    pkt.gps.timeUsec = msg.gps().time_usec();
    pkt.gps.latE7 = msg.gps().lat_e7();
    pkt.gps.lonE7 = msg.gps().lon_e7();
    pkt.gps.altMm = msg.gps().alt_mm();
    pkt.gps.velCms = msg.gps().vel_cms();
    pkt.gps.cogCdeg = msg.gps().cog_cdeg();
    pkt.gps.fixType = msg.gps().fix_type();
    pkt.gps.satellitesVisible = msg.gps().satellites_visible();

    emit telemetryReceived(pkt);
  }
}


} // namespace demo::client
