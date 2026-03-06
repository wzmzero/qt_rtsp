#include "app/main_window.h"

#include "database/database_service.h"
#include "media/record_worker.h"
#include "media/stream_worker.h"
#include "network/tcp_client_worker.h"
#include "repository/app_repository.h"

#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QMetaObject>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVideoSink>
#include <QVideoWidget>
#include <QVBoxLayout>

namespace {
double e7ToDegree(int e7) { return static_cast<double>(e7) / 10000000.0; }
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  settings_ = new QSettings("qt_rtsp_tcp_project", "qt_client", this);
  dataDir_ = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  dbPath_ = dataDir_ + "/client_data.sqlite";

  db_ = new demo::client::SQLiteDatabaseService(dbPath_);
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

  connect(tcp_, &demo::client::TcpClientWorker::logMessage, this, [this](const QString& msg) { appendLog("INFO", "tcp", msg); });
  connect(stream_, &demo::client::StreamWorker::logMessage, this, [this](const QString& msg) { appendLog("INFO", "stream", msg); });
  connect(recorder_, &demo::client::RecordWorker::logMessage, this, [this](const QString& msg) { appendLog("INFO", "record", msg); });
  connect(recorder_, &demo::client::RecordWorker::playbackIndexed, db_,
          &demo::client::SQLiteDatabaseService::insertPlaybackIndexAsync, Qt::QueuedConnection);

  tcpThread_.start();
  streamThread_.start();
  recordThread_.start();
  dbThread_.start();

  bool dbReady = false;
  QMetaObject::invokeMethod(db_, [&dbReady, this]() { dbReady = db_->initialize(); }, Qt::BlockingQueuedConnection);
  if (!dbReady) {
    appendLog("WARN", "db", "SQLite 初始化失败，将仅使用 QSettings");
  }

  config_ = repo_->loadConfig();
  if (config_.recordDir.isEmpty()) config_.recordDir = dataDir_ + "/records";
  applyConfigToUi(config_);

  if (!config_.windowGeometry.isEmpty()) restoreGeometry(config_.windowGeometry);
  if (!config_.windowState.isEmpty()) restoreState(config_.windowState);

  statusBar()->showMessage(QString("数据库: %1").arg(dbPath_));
  appendLog("INFO", "app", "MainWindow initialized");
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
  auto* pageMenu = menuBar()->addMenu("页面");
  auto* pageGroup = new QActionGroup(this);
  pageGroup->setExclusive(true);

  auto addPageAction = [this, pageMenu, pageGroup](const QString& name, int index) {
    auto* action = pageMenu->addAction(name);
    action->setCheckable(true);
    pageGroup->addAction(action);
    connect(action, &QAction::triggered, this, [this, index]() { pages_->setCurrentIndex(index); });
    return action;
  };

  auto* liveAction = addPageAction("实时模式", 0);
  addPageAction("回放模式", 1);
  addPageAction("日志查看", 2);
  addPageAction("事件查看", 3);
  liveAction->setChecked(true);

  auto* liveMenu = menuBar()->addMenu("实时功能");
  liveMenu->addAction("截图", this, &MainWindow::onCaptureScreenshot);

  auto* eventMenu = menuBar()->addMenu("事件功能");
  eventMenu->addAction("刷新事件", this, &MainWindow::onRefreshEvents);

  auto* helpMenu = menuBar()->addMenu("帮助");
  helpMenu->addAction("关于", [this] {
    QMessageBox::information(this, "关于", "Qt RTSP + TCP Client\nPhase1 可用版本");
  });
}

void MainWindow::setupUi() {
  setWindowTitle("Qt RTSP + TCP Client Demo");
  setWindowIcon(QIcon(":/icons/app_icon.xpm"));
  resize(1380, 860);
  setMinimumSize(960, 600);

  auto* root = new QWidget(this);
  auto* rootLayout = new QVBoxLayout(root);

  pages_ = new QStackedWidget(root);

  // 实时页
  auto* livePage = new QWidget(pages_);
  auto* liveLayout = new QHBoxLayout(livePage);

  auto* videoBox = new QGroupBox("实时视频", livePage);
  auto* videoLayout = new QVBoxLayout(videoBox);
  videoWidget_ = new QVideoWidget(videoBox);
  videoWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  videoLayout->addWidget(videoWidget_, 1);
  screenshotBtn_ = new QPushButton("截图", videoBox);
  connect(screenshotBtn_, &QPushButton::clicked, this, &MainWindow::onCaptureScreenshot);
  videoLayout->addWidget(screenshotBtn_, 0, Qt::AlignRight);

  auto* side = new QWidget(livePage);
  auto* sideLayout = new QVBoxLayout(side);

  auto* statusBox = new QGroupBox("状态面板", side);
  auto* statusLayout = new QVBoxLayout(statusBox);
  connLight_ = new QLabel(statusBox);
  connLight_->setFixedSize(14, 14);
  connLight_->setStyleSheet("background:#ef4444;border-radius:7px;");
  connState_ = new QLabel("连接状态: stopped", statusBox);
  auto* lightRow = new QHBoxLayout();
  lightRow->addWidget(new QLabel("连接灯:", statusBox));
  lightRow->addWidget(connLight_);
  lightRow->addStretch();
  statusLayout->addLayout(lightRow);
  statusLayout->addWidget(connState_);

  auto* rawGroup = new QGroupBox("未解析原始数据", statusBox);
  auto* rawLayout = new QVBoxLayout(rawGroup);
  tsLabel_ = new QLabel("时间戳: -", rawGroup);
  gpsRawLabel_ = new QLabel("GPS(E7): -", rawGroup);
  rawLayout->addWidget(tsLabel_);
  rawLayout->addWidget(gpsRawLabel_);

  auto* parsedGroup = new QGroupBox("解析结果", statusBox);
  auto* parsedLayout = new QVBoxLayout(parsedGroup);
  detectionLabel_ = new QLabel("检测结果: -", parsedGroup);
  gpsParsedLabel_ = new QLabel("GPS(度): -", parsedGroup);
  parsedLayout->addWidget(detectionLabel_);
  parsedLayout->addWidget(gpsParsedLabel_);

  statusLayout->addWidget(rawGroup);
  statusLayout->addWidget(parsedGroup);

  auto* cfgGroup = new QGroupBox("参数编辑区", side);
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
    if (!d.isEmpty()) recordDirEdit_->setText(d);
  });
  auto* recordRow = new QWidget(cfgGroup);
  auto* recordLayout = new QHBoxLayout(recordRow);
  recordLayout->setContentsMargins(0, 0, 0, 0);
  recordLayout->addWidget(recordDirEdit_);
  recordLayout->addWidget(browseBtn);

  auto* buttonRow = new QWidget(cfgGroup);
  auto* buttonLayout = new QHBoxLayout(buttonRow);
  buttonLayout->setContentsMargins(0, 0, 0, 0);
  auto* startBtn = new QPushButton("启动全部", buttonRow);
  auto* stopBtn = new QPushButton("停止全部", buttonRow);
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
  form->addRow("主题(保存后生效)", themeCombo_);
  form->addRow("控制", buttonRow);

  sideLayout->addWidget(statusBox, 3);
  sideLayout->addWidget(cfgGroup, 2);

  liveLayout->addWidget(videoBox, 3);
  liveLayout->addWidget(side, 2);

  pages_->addWidget(livePage);

  auto* playbackPage = new QWidget(pages_);
  auto* pbLayout = new QVBoxLayout(playbackPage);
  pbLayout->addWidget(new QLabel("回放模式（第一阶段占位）：请在录制目录查看录像与元数据。", playbackPage));
  pages_->addWidget(playbackPage);

  auto* logsPage = new QWidget(pages_);
  auto* logLayout = new QVBoxLayout(logsPage);
  auto* logFilterRow = new QWidget(logsPage);
  auto* logFilterLayout = new QHBoxLayout(logFilterRow);
  logFilterLayout->setContentsMargins(0, 0, 0, 0);
  logLevelFilter_ = new QComboBox(logsPage);
  logLevelFilter_->addItems({"ALL", "INFO", "WARN", "ERROR"});
  logTypeFilter_ = new QComboBox(logsPage);
  logTypeFilter_->addItems({"ALL", "app", "tcp", "stream", "record", "db", "event"});
  connect(logLevelFilter_, &QComboBox::currentTextChanged, this, &MainWindow::onLogFilterChanged);
  connect(logTypeFilter_, &QComboBox::currentTextChanged, this, &MainWindow::onLogFilterChanged);
  logFilterLayout->addWidget(new QLabel("级别", logsPage));
  logFilterLayout->addWidget(logLevelFilter_);
  logFilterLayout->addWidget(new QLabel("类型", logsPage));
  logFilterLayout->addWidget(logTypeFilter_);
  logFilterLayout->addStretch();
  logs_ = new QPlainTextEdit(logsPage);
  logs_->setReadOnly(true);
  logs_->setMaximumBlockCount(2000);
  logLayout->addWidget(logFilterRow);
  logLayout->addWidget(logs_, 1);
  pages_->addWidget(logsPage);

  auto* eventPage = new QWidget(pages_);
  auto* eventLayout = new QVBoxLayout(eventPage);
  auto* eventFilterRow = new QWidget(eventPage);
  auto* ef = new QHBoxLayout(eventFilterRow);
  ef->setContentsMargins(0, 0, 0, 0);
  eventLabelFilter_ = new QLineEdit(eventPage);
  eventFrom_ = new QDateTimeEdit(QDateTime::currentDateTime().addDays(-1), eventPage);
  eventTo_ = new QDateTimeEdit(QDateTime::currentDateTime().addSecs(60), eventPage);
  eventFrom_->setCalendarPopup(true);
  eventTo_->setCalendarPopup(true);
  auto* refreshBtn = new QPushButton("查询", eventPage);
  connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshEvents);
  ef->addWidget(new QLabel("标签", eventPage));
  ef->addWidget(eventLabelFilter_);
  ef->addWidget(new QLabel("开始", eventPage));
  ef->addWidget(eventFrom_);
  ef->addWidget(new QLabel("结束", eventPage));
  ef->addWidget(eventTo_);
  ef->addWidget(refreshBtn);

  eventTable_ = new QTableWidget(eventPage);
  eventTable_->setColumnCount(5);
  eventTable_->setHorizontalHeaderLabels({"时间", "标签", "置信度", "截图路径", "事件"});
  eventTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

  eventLayout->addWidget(eventFilterRow);
  eventLayout->addWidget(eventTable_, 1);
  pages_->addWidget(eventPage);

  rootLayout->addWidget(pages_, 1);
  setCentralWidget(root);

  connBlinkTimer_ = new QTimer(this);
  connBlinkTimer_->setInterval(450);
  connect(connBlinkTimer_, &QTimer::timeout, this, &MainWindow::blinkConnectionIndicator);
}

void MainWindow::onStartAll() {
  config_ = collectConfigFromUi();
  repo_->saveConfig(config_);

  QMetaObject::invokeMethod(tcp_, "start", Qt::QueuedConnection, Q_ARG(QString, config_.tcpHost),
                            Q_ARG(quint16, config_.tcpPort), Q_ARG(int, config_.reconnectIntervalMs));
  QMetaObject::invokeMethod(stream_, "start", Qt::QueuedConnection, Q_ARG(QUrl, QUrl(config_.rtspUrl)));
  if (config_.recordEnabled) QMetaObject::invokeMethod(recorder_, "start", Qt::QueuedConnection, Q_ARG(QString, config_.recordDir));

  statusBar()->showMessage(QString("运行中 | DB: %1").arg(dbPath_));
  appendLog("INFO", "app", "Workers started");
}

void MainWindow::onStopAll() {
  QMetaObject::invokeMethod(tcp_, "stop", Qt::QueuedConnection);
  QMetaObject::invokeMethod(stream_, "stop", Qt::QueuedConnection);
  QMetaObject::invokeMethod(recorder_, "stop", Qt::QueuedConnection);
  statusBar()->showMessage(QString("已停止 | DB: %1").arg(dbPath_));
  appendLog("INFO", "app", "Workers stopped");
}

void MainWindow::onSaveConfig() {
  config_ = collectConfigFromUi();
  config_.windowGeometry = saveGeometry();
  config_.windowState = saveState();
  repo_->saveConfig(config_);
  applyTheme(config_.theme);
  appendLog("INFO", "app", "Config saved to QSettings + SQLite");
  QMessageBox::information(this, "保存成功", "配置已保存，主题已应用。");
}

void MainWindow::onTelemetry(const demo::client::TelemetryPacket& pkt) {
  lastPkt_ = pkt;
  tsLabel_->setText(QString("时间戳: source=%1 sent=%2 recv=%3").arg(pkt.detection.sourceTsMs).arg(pkt.sentTsMs).arg(pkt.recvTsMs));
  detectionLabel_->setText(QString("检测结果: %1 (%.2f)").arg(pkt.detection.label).arg(pkt.detection.confidence, 0, 'f', 2));
  gpsRawLabel_->setText(QString("GPS(E7): lat=%1 lon=%2 alt_mm=%3 sat=%4")
                            .arg(pkt.gps.latE7)
                            .arg(pkt.gps.lonE7)
                            .arg(pkt.gps.altMm)
                            .arg(pkt.gps.satellitesVisible));
  gpsParsedLabel_->setText(QString("GPS(度): lat=%1 lon=%2")
                               .arg(e7ToDegree(pkt.gps.latE7), 0, 'f', 7)
                               .arg(e7ToDegree(pkt.gps.lonE7), 0, 'f', 7));

  repo_->appendTelemetry(pkt);
}

void MainWindow::onFrame(const QVideoFrame& frame, qint64 tsMs) {
  if (videoWidget_ && videoWidget_->videoSink()) {
    videoWidget_->videoSink()->setVideoFrame(frame);
  }

  if (!config_.recordEnabled) return;
  demo::client::RecordItem item;
  item.frameTsMs = tsMs;
  item.frame = frame;
  item.telemetry = lastPkt_;
  QMetaObject::invokeMethod(recorder_, "enqueue", Qt::QueuedConnection, Q_ARG(demo::client::RecordItem, item));
}

void MainWindow::onConnectionStateChanged(const QString& state) {
  connStateValue_ = state;
  connState_->setText("连接状态: " + state);
  const bool active = state.contains("connected", Qt::CaseInsensitive) || state.contains("running", Qt::CaseInsensitive);
  if (active) {
    connBlinkTimer_->start();
  } else {
    connBlinkTimer_->stop();
    connLight_->setStyleSheet("background:#ef4444;border-radius:7px;");
  }
}

void MainWindow::blinkConnectionIndicator() {
  connBlinkOn_ = !connBlinkOn_;
  connLight_->setStyleSheet(connBlinkOn_ ? "background:#22c55e;border-radius:7px;" : "background:#166534;border-radius:7px;");
}

QString MainWindow::saveScreenshotWithMetadata(const QString& reasonTag) {
  const QString dir = QDir(config_.recordDir).filePath("screenshots");
  QDir().mkpath(dir);
  const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
  const QString path = QDir(dir).filePath(QString("shot_%1_%2.jpg").arg(nowMs).arg(reasonTag));

  const auto pix = videoWidget_->grab();
  if (pix.isNull() || !pix.save(path, "JPG")) {
    appendLog("ERROR", "event", "截图失败");
    return {};
  }

  const bool isEvent = (lastPkt_.detection.label.compare("person", Qt::CaseInsensitive) == 0);
  QMetaObject::invokeMethod(db_, "insertSnapshotEventAsync", Qt::QueuedConnection,
                            Q_ARG(demo::client::TelemetryPacket, lastPkt_), Q_ARG(QString, path),
                            Q_ARG(QString, reasonTag), Q_ARG(bool, isEvent));

  appendLog("INFO", "event", QString("截图保存: %1").arg(path));
  return path;
}

void MainWindow::onCaptureScreenshot() { saveScreenshotWithMetadata("manual"); }

void MainWindow::onRefreshEvents() {
  QList<demo::client::EventRecord> result;
  const auto label = eventLabelFilter_->text().trimmed();
  const qint64 fromMs = eventFrom_->dateTime().toMSecsSinceEpoch();
  const qint64 toMs = eventTo_->dateTime().toMSecsSinceEpoch();

  QMetaObject::invokeMethod(
      db_,
      [this, &result, label, fromMs, toMs]() { result = db_->queryEvents(label, fromMs, toMs, 300); },
      Qt::BlockingQueuedConnection);

  eventTable_->setRowCount(result.size());
  for (int i = 0; i < result.size(); ++i) {
    const auto& r = result[i];
    eventTable_->setItem(i, 0, new QTableWidgetItem(QDateTime::fromMSecsSinceEpoch(r.tsMs).toString("yyyy-MM-dd HH:mm:ss")));
    eventTable_->setItem(i, 1, new QTableWidgetItem(r.label));
    eventTable_->setItem(i, 2, new QTableWidgetItem(QString::number(r.confidence, 'f', 2)));
    eventTable_->setItem(i, 3, new QTableWidgetItem(r.screenshotPath));
    eventTable_->setItem(i, 4, new QTableWidgetItem(r.isTargetEvent ? "是" : "否"));
  }
  appendLog("INFO", "event", QString("事件查询完成: %1 条").arg(result.size()));
}

void MainWindow::onLogFilterChanged() { refreshLogView(); }

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
    qApp->setStyleSheet("QMainWindow{background:#1e293b;color:#e2e8f0;} QGroupBox{font-weight:600;border:1px solid #475569;border-radius:8px;margin-top:12px;padding-top:8px;} QLabel{color:#e2e8f0;} QLineEdit,QSpinBox,QComboBox,QDateTimeEdit,QPlainTextEdit,QTableWidget{background:#0f172a;color:#e2e8f0;border:1px solid #334155;padding:4px;} QPushButton{background:#334155;color:#e2e8f0;padding:6px 12px;border-radius:4px;} QPushButton:hover{background:#475569;}");
  }
}

void MainWindow::appendLog(const QString& level, const QString& type, const QString& msg) {
  LogEntry e;
  e.tsMs = QDateTime::currentMSecsSinceEpoch();
  e.level = level;
  e.type = type;
  e.message = msg;
  logEntries_.push_back(e);

  if (db_) {
    QMetaObject::invokeMethod(db_, "insertAppLogAsync", Qt::QueuedConnection, Q_ARG(qint64, e.tsMs),
                              Q_ARG(QString, e.level), Q_ARG(QString, e.type), Q_ARG(QString, e.message));
  }

  refreshLogView();
}

void MainWindow::refreshLogView() {
  if (!logs_) return;
  const QString lv = logLevelFilter_ ? logLevelFilter_->currentText() : "ALL";
  const QString tp = logTypeFilter_ ? logTypeFilter_->currentText() : "ALL";

  logs_->clear();
  for (const auto& e : logEntries_) {
    if (lv != "ALL" && e.level != lv) continue;
    if (tp != "ALL" && e.type != tp) continue;
    logs_->appendPlainText(QString("[%1][%2][%3] %4")
                               .arg(QDateTime::fromMSecsSinceEpoch(e.tsMs).toString("HH:mm:ss"))
                               .arg(e.level)
                               .arg(e.type)
                               .arg(e.message));
  }
}
