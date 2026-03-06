#pragma once

#include "core/types.h"

#include <QMainWindow>
#include <QVideoFrame>
#include <QThread>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QPlainTextEdit;
class QVideoWidget;
QT_END_NAMESPACE

namespace demo::client {
class TcpClientWorker;
class StreamWorker;
class RecordWorker;
}

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override;

private slots:
  void onStartAll();
  void onStopAll();
  void onTelemetry(const demo::client::TelemetryPacket& pkt);
  void onFrame(const QVideoFrame& frame, qint64 tsMs);
  void onLog(const QString& msg);

private:
  void setupUi();

  QLabel* status_{nullptr};
  QLabel* telemetry_{nullptr};
  QPushButton* startBtn_{nullptr};
  QPushButton* stopBtn_{nullptr};
  QPlainTextEdit* logs_{nullptr};
  QVideoWidget* videoWidget_{nullptr};

  QThread tcpThread_;
  QThread streamThread_;
  QThread recordThread_;

  demo::client::TcpClientWorker* tcp_{nullptr};
  demo::client::StreamWorker* stream_{nullptr};
  demo::client::RecordWorker* recorder_{nullptr};
  demo::client::TelemetryPacket lastPkt_{};
};
