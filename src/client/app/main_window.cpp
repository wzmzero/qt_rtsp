#include "app/main_window.h"

#include "media/record_worker.h"
#include "media/stream_worker.h"
#include "network/tcp_client_worker.h"

#include <QDateTime>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QMetaObject>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QVideoSink>
#include <QVideoWidget>
#include <QWidget>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  setupUi();

  tcp_ = new demo::client::TcpClientWorker();
  stream_ = new demo::client::StreamWorker();
  recorder_ = new demo::client::RecordWorker();

  tcp_->moveToThread(&tcpThread_);
  stream_->moveToThread(&streamThread_);
  recorder_->moveToThread(&recordThread_);

  connect(&tcpThread_, &QThread::finished, tcp_, &QObject::deleteLater);
  connect(&streamThread_, &QThread::finished, stream_, &QObject::deleteLater);
  connect(&recordThread_, &QThread::finished, recorder_, &QObject::deleteLater);

  connect(tcp_, &demo::client::TcpClientWorker::telemetryReceived, this, &MainWindow::onTelemetry);
  connect(stream_, &demo::client::StreamWorker::frameArrived, this, &MainWindow::onFrame);

  connect(tcp_, &demo::client::TcpClientWorker::logMessage, this, &MainWindow::onLog);
  connect(stream_, &demo::client::StreamWorker::logMessage, this, &MainWindow::onLog);
  connect(recorder_, &demo::client::RecordWorker::logMessage, this, &MainWindow::onLog);

  tcpThread_.start();
  streamThread_.start();
  recordThread_.start();

  onLog("MainWindow initialized");
}

MainWindow::~MainWindow() {
  onStopAll();

  tcpThread_.quit();
  streamThread_.quit();
  recordThread_.quit();

  tcpThread_.wait();
  streamThread_.wait();
  recordThread_.wait();
}

void MainWindow::onStartAll() {
  startBtn_->setEnabled(false);
  stopBtn_->setEnabled(true);
  status_->setText("Status: running");

  const QString outBase = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  const QString outDir = outBase + "/records";

  QMetaObject::invokeMethod(tcp_, "start", Qt::QueuedConnection, Q_ARG(QString, "127.0.0.1"), Q_ARG(quint16, 9000));
  QMetaObject::invokeMethod(stream_, "start", Qt::QueuedConnection,
                            Q_ARG(QUrl, QUrl("rtsp://127.0.0.1:8554/live")));
  QMetaObject::invokeMethod(recorder_, "start", Qt::QueuedConnection, Q_ARG(QString, outDir));

  onLog(QString("Workers started, output=%1").arg(QDir::toNativeSeparators(outDir)));
}

void MainWindow::onStopAll() {
  QMetaObject::invokeMethod(tcp_, "stop", Qt::QueuedConnection);
  QMetaObject::invokeMethod(stream_, "stop", Qt::QueuedConnection);
  QMetaObject::invokeMethod(recorder_, "stop", Qt::QueuedConnection);

  startBtn_->setEnabled(true);
  stopBtn_->setEnabled(false);
  status_->setText("Status: stopped");
}

void MainWindow::onTelemetry(const demo::client::TelemetryPacket& pkt) {
  lastPkt_ = pkt;
  const auto oneWayLatency = pkt.recvTsMs - pkt.sentTsMs;
  telemetry_->setText(
      QString("det=%1(%.2f) gps=%2,%3 sats=%4 latency=%5ms")
          .arg(pkt.detection.label)
          .arg(pkt.detection.confidence, 0, 'f', 2)
          .arg(pkt.gps.latE7)
          .arg(pkt.gps.lonE7)
          .arg(pkt.gps.satellitesVisible)
          .arg(oneWayLatency));
}

void MainWindow::onFrame(const QVideoFrame& frame, qint64 tsMs) {
  if (videoWidget_ && videoWidget_->videoSink()) {
    videoWidget_->videoSink()->setVideoFrame(frame);
  }

  demo::client::RecordItem item;
  item.frameTsMs = tsMs;
  item.frame = frame;
  item.telemetry = lastPkt_;

  QMetaObject::invokeMethod(recorder_, "enqueue", Qt::QueuedConnection, Q_ARG(demo::client::RecordItem, item));
}

void MainWindow::onLog(const QString& msg) {
  logs_->appendPlainText(QString("[%1] %2").arg(QDateTime::currentDateTime().toString("HH:mm:ss"), msg));
}

void MainWindow::setupUi() {
  setWindowTitle("Qt RTSP + TCP Client Demo");
  resize(980, 680);

  auto* root = new QWidget(this);
  auto* layout = new QVBoxLayout(root);
  auto* btnRow = new QHBoxLayout();

  status_ = new QLabel("Status: stopped", root);
  telemetry_ = new QLabel("telemetry: n/a", root);
  startBtn_ = new QPushButton("Start", root);
  stopBtn_ = new QPushButton("Stop", root);
  stopBtn_->setEnabled(false);

  btnRow->addWidget(startBtn_);
  btnRow->addWidget(stopBtn_);
  btnRow->addStretch(1);
  btnRow->addWidget(status_);

  videoWidget_ = new QVideoWidget(root);
  videoWidget_->setMinimumHeight(360);

  logs_ = new QPlainTextEdit(root);
  logs_->setReadOnly(true);
  logs_->setMaximumBlockCount(1000);

  layout->addLayout(btnRow);
  layout->addWidget(telemetry_);
  layout->addWidget(videoWidget_, 1);
  layout->addWidget(logs_, 1);

  setCentralWidget(root);

  connect(startBtn_, &QPushButton::clicked, this, &MainWindow::onStartAll);
  connect(stopBtn_, &QPushButton::clicked, this, &MainWindow::onStopAll);
}
