#include "network/tcp_client_worker.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

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
    pkt.rawJsonLine = QString::fromUtf8(line);

    const auto det = root.value("detection").toObject();
    pkt.detection.label = det.value("label").toString();
    pkt.detection.confidence = det.value("confidence").toDouble();
    pkt.detection.sourceTsMs = static_cast<qint64>(det.value("source_ts_ms").toDouble());

    auto parseObjects = [&pkt](const QJsonValue& v) {
      const auto arr = v.toArray();
      for (const auto& item : arr) {
        const auto o = item.toObject();
        demo::client::DetectionObject obj;
        obj.label = o.value("label").toString();
        obj.confidence = o.value("confidence").toDouble();
        if (o.contains("cx") && o.contains("cy") && o.contains("w") && o.contains("h")) {
          obj.bbox = QRectF(o.value("cx").toDouble(), o.value("cy").toDouble(),
                            o.value("w").toDouble(), o.value("h").toDouble());
        } else if (o.contains("bbox") && o.value("bbox").isArray()) {
          const auto b = o.value("bbox").toArray();
          if (b.size() >= 4) {
            obj.bbox = QRectF(b.at(0).toDouble(), b.at(1).toDouble(), b.at(2).toDouble(), b.at(3).toDouble());
          }
        }
        pkt.detection.objects.push_back(obj);
      }
    };

    parseObjects(root.value("objects"));
    parseObjects(det.value("objects"));

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
