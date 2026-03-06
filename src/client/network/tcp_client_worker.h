#pragma once

#include "core/types.h"

#include <QObject>
#include <QTcpSocket>

namespace demo::client {

class TcpClientWorker : public QObject {
  Q_OBJECT
public:
  explicit TcpClientWorker(QObject* parent = nullptr);

public slots:
  void start(const QString& host, quint16 port);
  void stop();

signals:
  void telemetryReceived(const demo::client::TelemetryPacket& packet);
  void logMessage(const QString& msg);

private slots:
  void onReadyRead();
  void onConnected();
  void onDisconnected();

private:
  QTcpSocket* socket_{nullptr};
  QByteArray buf_;
};

} // namespace demo::client
