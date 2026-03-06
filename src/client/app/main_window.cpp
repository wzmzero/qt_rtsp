#include "app/main_window.h"

#include "database/database_service.h"
#include "media/record_worker.h"
#include "media/stream_worker.h"
#include "network/tcp_client_worker.h"
#include "repository/app_repository.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QMetaObject>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QToolBar>
#include <QUrl>
#include <QVideoSink>
#include <QVideoWidget>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  settings_ = new QSettings("qt_rtsp_tcp_project", "qt_client", this);
  const auto dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

  db_ = new demo::client::SQLiteDatabaseService(dataDir + "/client_data.sqlite");
  repo_ = new demo::client::AppRepository(db_, settings_, this);

  setupUi();
  setupMenus();

  tcp_ = new demo::client::TcpClientWorker();
  stream_ = new demo::client::StreamWorker();
  recorder_ = new demo::client::RecordWorker();

  tcp_->moveToThread(&tcpThread_);
  stream_->moveToThread(&streamThread_);
  recorder_->moveToThread(&recordThread_);
  db_->moveToThread(&dbThread_);

  connect(&tcpThread_, &QThread::finished, tcp_, &QObject::deleteLater);
  connect(&streamThread_, &QThread::finished, stream_, &QObject::deleteLater);
  connect(&recordThread_, &QThread::finished, recorder_, &QObject::deleteLater);
  connect(&dbThread_, &QThread::finished, db_, &QObject::deleteLater);

  connect(tcp_, &demo::client::TcpClientWorker::telemetryReceived, this, &MainWindow::onTelemetry);
  connect(tcp_, &demo::client::TcpClientWorker::connectionStateChanged, this, &MainWindow::onConnectionStateChanged);
  connect(stream_, &demo::client::StreamWorker::frameArrived, this, &MainWindow::onFrame);

  connect(tcp_, &demo::client::TcpClientWorker::logMessage, this, &MainWindow::onLog);
  connect(stream_, &demo::client::StreamWorker::logMessage, this, &MainWindow::onLog);
  connect(recorder_, &demo::client::RecordWorker::logMessage, this, &MainWindow::onLog);

  tcpThread_.start();
  streamThread_.start();
  recordThread_.start();
  dbThread_.start();

  bool dbReady = false;
  QMetaObject::invokeMethod(
      db_,
      [&dbReady, this]() {
        dbReady = db_->initialize();
      },
      Qt::BlockingQueuedConnection);
  if (!dbReady) {
    onLog("WARNING: SQLite 初始化失败，配置/结果将仅保存在 QSettings");
  }

  config_ = repo_->loadConfig();
  if (config_.recordDir.isEmpty()) {
    config_.recordDir = dataDir + "/records";
  }
  applyConfigToUi(config_);

  if (!config_.windowGeometry.isEmpty()) {
    restoreGeometry(config_.windowGeometry);
  }
  if (!config_.windowState.isEmpty()) {
    restoreState(config_.windowState);
  }

  onLog("MainWindow initialized");
}

MainWindow::~MainWindow() {
  onStopAll();

  config_ = collectConfigFromUi();
  config_.windowGeometry = saveGeometry();
  config_.windowState = saveState();
  repo_->saveConfig(config_);

  tcpThread_.quit();
  streamThread_.quit();
  recordThread_.quit();
  dbThread_.quit();

  tcpThread_.wait();
  streamThread_.wait();
  recordThread_.wait();
  dbThread_.wait();
}

void MainWindow::setupMenus() {
  auto* fileMenu = menuBar()->addMenu("文件");
  auto* saveAction = fileMenu->addAction("保存配置");
  auto* exitAction = fileMenu->addAction("退出");

  auto* connMenu = menuBar()->addMenu("连接");
  auto* startAction = connMenu->addAction("启动全部");
  auto* stopAction = connMenu->addAction("停止全部");

  auto* viewMenu = menuBar()->addMenu("视图");
  viewMenu->addAction("暗色主题", [this] {
    themeCombo_->setCurrentText("dark");
    onSaveConfig();
  });
  viewMenu->addAction("浅色主题", [this] {
    themeCombo_->setCurrentText("light");
    onSaveConfig();
  });

  auto* helpMenu = menuBar()->addMenu("帮助");
  helpMenu->addAction("关于", [this] { QMessageBox::information(this, "关于", "Qt RTSP + TCP Client\nUI可配置版"); });

  auto* toolBar = addToolBar("main");
  toolBar->addAction(startAction);
  toolBar->addAction(stopAction);
  toolBar->addAction(saveAction);

  connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveConfig);
  connect(exitAction, &QAction::triggered, this, &QWidget::close);
  connect(startAction, &QAction::triggered, this, &MainWindow::onStartAll);
  connect(stopAction, &QAction::triggered, this, &MainWindow::onStopAll);
}

void MainWindow::setupUi() {
  setWindowTitle("Qt RTSP + TCP Client Demo");
  resize(1360, 820);

  auto* root = new QWidget(this);
  auto* rootLayout = new QVBoxLayout(root);

  auto* splitter = new QSplitter(Qt::Horizontal, root);

  auto* videoPane = new QGroupBox("实时视频", splitter);
  auto* videoLayout = new QVBoxLayout(videoPane);
  videoWidget_ = new QVideoWidget(videoPane);
  videoWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  videoWidget_->setMinimumSize(900, 520);
  videoLayout->addWidget(videoWidget_, 1, Qt::AlignCenter);

  auto* sidePane = new QWidget(splitter);
  auto* sideLayout = new QVBoxLayout(sidePane);

  auto* statusBox = new QGroupBox("状态面板", sidePane);
  auto* statusLayout = new QVBoxLayout(statusBox);
  connState_ = new QLabel("连接状态: stopped", statusBox);
  tsLabel_ = new QLabel("时间戳: -", statusBox);
  detectionLabel_ = new QLabel("检测结果: -", statusBox);
  gpsLabel_ = new QLabel("GPS: -", statusBox);
  statusLayout->addWidget(connState_);
  statusLayout->addWidget(tsLabel_);
  statusLayout->addWidget(detectionLabel_);
  statusLayout->addWidget(gpsLabel_);

  auto* cfgGroup = new QGroupBox("参数编辑区", sidePane);
  auto* form = new QFormLayout(cfgGroup);

  rtspEdit_ = new QLineEdit(cfgGroup);
  tcpHostEdit_ = new QLineEdit(cfgGroup);
  tcpPortSpin_ = new QSpinBox(cfgGroup);
  tcpPortSpin_->setRange(1, 65535);
  reconnectSpin_ = new QSpinBox(cfgGroup);
  reconnectSpin_->setRange(500, 60000);
  reconnectSpin_->setSingleStep(500);
  recordDirEdit_ = new QLineEdit(cfgGroup);
  recordEnabledCheck_ = new QCheckBox("启用录制", cfgGroup);
  themeCombo_ = new QComboBox(cfgGroup);
  themeCombo_->addItems({"dark", "light"});

  auto* browseBtn = new QPushButton("浏览", cfgGroup);
  connect(browseBtn, &QPushButton::clicked, this, [this] {
    const auto d = QFileDialog::getExistingDirectory(this, "选择录制目录", recordDirEdit_->text());
    if (!d.isEmpty()) {
      recordDirEdit_->setText(d);
    }
  });

  auto* recordRow = new QWidget(cfgGroup);
  auto* recordLayout = new QHBoxLayout(recordRow);
  recordLayout->setContentsMargins(0, 0, 0, 0);
  recordLayout->addWidget(recordDirEdit_);
  recordLayout->addWidget(browseBtn);

  auto* buttonRow = new QWidget(cfgGroup);
  auto* buttonLayout = new QHBoxLayout(buttonRow);
  buttonLayout->setContentsMargins(0, 0, 0, 0);
  auto* startBtn = new QPushButton("启动", buttonRow);
  auto* stopBtn = new QPushButton("停止", buttonRow);
  auto* saveCfgBtn = new QPushButton("保存配置", buttonRow);
  buttonLayout->addWidget(startBtn);
  buttonLayout->addWidget(stopBtn);
  buttonLayout->addWidget(saveCfgBtn);

  connect(startBtn, &QPushButton::clicked, this, &MainWindow::onStartAll);
  connect(stopBtn, &QPushButton::clicked, this, &MainWindow::onStopAll);
  connect(saveCfgBtn, &QPushButton::clicked, this, &MainWindow::onSaveConfig);

  form->addRow("RTSP URL", rtspEdit_);
  form->addRow("TCP Host", tcpHostEdit_);
  form->addRow("TCP Port", tcpPortSpin_);
  form->addRow("重连间隔(ms)", reconnectSpin_);
  form->addRow("录制路径", recordRow);
  form->addRow("录制开关", recordEnabledCheck_);
  form->addRow("主题", themeCombo_);
  form->addRow("控制", buttonRow);

  sideLayout->addWidget(statusBox);
  sideLayout->addWidget(cfgGroup, 1);

  splitter->addWidget(videoPane);
  splitter->addWidget(sidePane);
  splitter->setStretchFactor(0, 4);
  splitter->setStretchFactor(1, 1);

  logs_ = new QPlainTextEdit(root);
  logs_->setReadOnly(true);
  logs_->setMaximumBlockCount(1500);
  logs_->setMinimumHeight(160);

  rootLayout->addWidget(splitter, 1);
  rootLayout->addWidget(logs_);
  setCentralWidget(root);

  statusBar()->showMessage("就绪");
}

void MainWindow::onStartAll() {
  config_ = collectConfigFromUi();
  repo_->saveConfig(config_);

  QMetaObject::invokeMethod(tcp_, "start", Qt::QueuedConnection, Q_ARG(QString, config_.tcpHost),
                            Q_ARG(quint16, config_.tcpPort), Q_ARG(int, config_.reconnectIntervalMs));
  QMetaObject::invokeMethod(stream_, "start", Qt::QueuedConnection, Q_ARG(QUrl, QUrl(config_.rtspUrl)));

  if (config_.recordEnabled) {
    QMetaObject::invokeMethod(recorder_, "start", Qt::QueuedConnection, Q_ARG(QString, config_.recordDir));
  }

  statusBar()->showMessage("运行中");
  onLog("Workers started");
}

void MainWindow::onStopAll() {
  QMetaObject::invokeMethod(tcp_, "stop", Qt::QueuedConnection);
  QMetaObject::invokeMethod(stream_, "stop", Qt::QueuedConnection);
  QMetaObject::invokeMethod(recorder_, "stop", Qt::QueuedConnection);
  statusBar()->showMessage("已停止");
}

void MainWindow::onSaveConfig() {
  config_ = collectConfigFromUi();
  config_.windowGeometry = saveGeometry();
  config_.windowState = saveState();
  repo_->saveConfig(config_);
  applyTheme(config_.theme);
  onLog("Config saved to QSettings + SQLite");
}

void MainWindow::onTelemetry(const demo::client::TelemetryPacket& pkt) {
  lastPkt_ = pkt;
  tsLabel_->setText(QString("时间戳: source=%1 sent=%2 recv=%3").arg(pkt.detection.sourceTsMs).arg(pkt.sentTsMs).arg(pkt.recvTsMs));
  detectionLabel_->setText(QString("检测结果: %1 (%.2f)").arg(pkt.detection.label).arg(pkt.detection.confidence, 0, 'f', 2));
  gpsLabel_->setText(QString("GPS: %1,%2 alt=%3 sat=%4").arg(pkt.gps.latE7).arg(pkt.gps.lonE7).arg(pkt.gps.altMm).arg(pkt.gps.satellitesVisible));

  repo_->appendTelemetry(pkt);
}

void MainWindow::onFrame(const QVideoFrame& frame, qint64 tsMs) {
  if (videoWidget_ && videoWidget_->videoSink()) {
    videoWidget_->videoSink()->setVideoFrame(frame);
  }

  if (!config_.recordEnabled) {
    return;
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

void MainWindow::onConnectionStateChanged(const QString& state) { connState_->setText("连接状态: " + state); }

demo::client::AppConfig MainWindow::collectConfigFromUi() const {
  demo::client::AppConfig cfg;
  cfg.rtspUrl = rtspEdit_->text().trimmed();
  cfg.tcpHost = tcpHostEdit_->text().trimmed();
  cfg.tcpPort = static_cast<quint16>(tcpPortSpin_->value());
  cfg.reconnectIntervalMs = reconnectSpin_->value();
  cfg.recordDir = recordDirEdit_->text().trimmed();
  cfg.recordEnabled = recordEnabledCheck_->isChecked();
  cfg.theme = themeCombo_->currentText();
  return cfg;
}

void MainWindow::applyConfigToUi(const demo::client::AppConfig& cfg) {
  rtspEdit_->setText(cfg.rtspUrl);
  tcpHostEdit_->setText(cfg.tcpHost);
  tcpPortSpin_->setValue(cfg.tcpPort);
  reconnectSpin_->setValue(cfg.reconnectIntervalMs);
  recordDirEdit_->setText(cfg.recordDir);
  recordEnabledCheck_->setChecked(cfg.recordEnabled);
  themeCombo_->setCurrentText(cfg.theme);
  applyTheme(cfg.theme);
}

void MainWindow::applyTheme(const QString& theme) {
  if (theme == "light") {
    qApp->setStyleSheet("QMainWindow{background:#f3f5f7;} QGroupBox{font-weight:600;border:1px solid #c9d3e0;border-radius:8px;margin-top:12px;padding-top:8px;} QPlainTextEdit{background:#fff;} QLabel{color:#1f2937;}");
  } else {
    qApp->setStyleSheet("QMainWindow{background:#1e293b;color:#e2e8f0;} QGroupBox{font-weight:600;border:1px solid #475569;border-radius:8px;margin-top:12px;padding-top:8px;} QLabel{color:#e2e8f0;} QLineEdit,QSpinBox,QComboBox,QPlainTextEdit{background:#0f172a;color:#e2e8f0;border:1px solid #334155;padding:4px;} QPushButton{background:#334155;color:#e2e8f0;padding:6px 12px;border-radius:4px;} QPushButton:hover{background:#475569;}");
  }
}
