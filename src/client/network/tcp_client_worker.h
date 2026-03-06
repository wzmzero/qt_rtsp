#pragma once

#include "core/types.h"

#include <QObject>
#include <QTcpSocket>
#include <QTimer>

namespace demo::client {

class TcpClientWorker : public QObject {
  Q_OBJECT
public:
  explicit TcpClientWorker(QObject* parent = nullptr);

public slots:
  void start(const QString& host, quint16 port, int reconnectIntervalMs);
  void stop();

signals:
  void telemetryReceived(const demo::client::TelemetryPacket& packet);
  void logMessage(const QString& msg);
  void connectionStateChanged(const QString& state);

private slots:
  void onReadyRead();
  void onConnected();
  void onDisconnected();
  void reconnect();

private:
  QTcpSocket* socket_{nullptr};
  QTimer* reconnectTimer_{nullptr};
  QByteArray buf_;
  QString host_;
  quint16 port_{0};
  int reconnectIntervalMs_{3000};
  bool running_{false};
};

} // namespace demo::client
