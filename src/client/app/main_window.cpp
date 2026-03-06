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
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QKeySequence>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QMetaObject>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QTextStream>
#include <QToolButton>
#include <QUrl>
#include <QVideoSink>
#include <QVideoWidget>
#include <QVBoxLayout>

namespace {
double e7ToDegree(int e7) { return static_cast<double>(e7) / 10000000.0; }
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  projectRootDir_ = QDir::currentPath();
  const QString confDir = QDir(projectRootDir_).filePath("./conf");
  const QString dbDir = QDir(projectRootDir_).filePath("./config");
  QDir().mkpath(confDir);
  QDir().mkpath(dbDir);
  settings_ = new QSettings(QDir(confDir).filePath("client.ini"), QSettings::IniFormat, this);
  dataDir_ = resolvePath(settings_->value("storage/data_dir").toString(), "./data");
  logsDir_ = resolvePath(settings_->value("storage/logs_dir").toString(), "./logs");
  snapshotsDir_ = resolvePath(settings_->value("storage/snapshots_dir").toString(), "./snapshots");
  dbPath_ = QDir(dbDir).filePath("client_cfg.db");
  dbDisplayPath_ = "./config/client_cfg.db";

  const QString legacyDbPath = QDir(projectRootDir_).filePath("./data/client_data.sqlite");
  if (!QFile::exists(dbPath_) && QFile::exists(legacyDbPath)) {
    QDir().mkpath(QFileInfo(dbPath_).absolutePath());
    if (QFile::copy(legacyDbPath, dbPath_)) {
      qInfo() << "migrated legacy db:" << legacyDbPath << "->" << dbPath_;
    } else {
      qWarning() << "failed to migrate legacy db:" << legacyDbPath << "->" << dbPath_;
    }
  }

  logFilePath_ = QDir(logsDir_).filePath("qt_client.log");
  ensureRuntimeDirs();

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
  if (config_.recordDir.isEmpty()) config_.recordDir = "./recordings";
  config_.recordDir = resolvePath(config_.recordDir, "./recordings");
  applyConfigToUi(config_);

  if (!config_.windowGeometry.isEmpty()) restoreGeometry(config_.windowGeometry);
  if (!config_.windowState.isEmpty()) restoreState(config_.windowState);

  statusBar()->showMessage(QString("工程目录: %1 | DB: %2").arg(projectRootDir_, dbDisplayPath_));
  appendLog("INFO", "app", QString("MainWindow initialized, DB display path=%1").arg(dbDisplayPath_));

  if (qEnvironmentVariableIsSet("QT_CLIENT_AUTOTEST")) {
    appendLog("INFO", "app", "AUTOTEST enabled: start->save->stop sequence");
    QTimer::singleShot(400, this, &MainWindow::onStartAll);
    QTimer::singleShot(2000, this, &MainWindow::onSaveConfig);
    QTimer::singleShot(3400, this, &MainWindow::onStopAll);
    QTimer::singleShot(4300, qApp, &QCoreApplication::quit);
  }
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
  auto* fileMenu = menuBar()->addMenu("文件(File)");
  fileMenu->addAction("保存配置", this, &MainWindow::onSaveConfig, QKeySequence::Save);
  fileMenu->addSeparator();
  fileMenu->addAction("退出", this, &QWidget::close, QKeySequence::Quit);

  auto* pageMenu = menuBar()->addMenu("页面(Page)");
  auto* pageGroup = new QActionGroup(this);
  pageGroup->setExclusive(true);

  auto addPageAction = [this, pageMenu, pageGroup](const QString& name, int index) {
    auto* action = pageMenu->addAction(name);
    action->setCheckable(true);
    pageGroup->addAction(action);
    connect(action, &QAction::triggered, this, [this, index]() { pages_->setCurrentIndex(index); });
    return action;
  };

  auto* liveAction = addPageAction("实时", 0);
  addPageAction("回放", 1);
  addPageAction("日志", 2);
  addPageAction("事件", 3);
  liveAction->setChecked(true);

  auto* functionMenu = menuBar()->addMenu("功能(Function)");
  functionMenu->addAction("截图", this, &MainWindow::onCaptureScreenshot);
  functionMenu->addAction("刷新事件", this, &MainWindow::onRefreshEvents);
  functionMenu->addSeparator();

  auto* themeMenu = functionMenu->addMenu("主题");
  auto* themeGroup = new QActionGroup(this);
  themeGroup->setExclusive(true);

  auto* darkAction = themeMenu->addAction("暗");
  darkAction->setCheckable(true);
  auto* lightAction = themeMenu->addAction("明");
  lightAction->setCheckable(true);
  themeGroup->addAction(darkAction);
  themeGroup->addAction(lightAction);

  if (config_.theme == "light") {
    lightAction->setChecked(true);
  } else {
    darkAction->setChecked(true);
  }

  connect(darkAction, &QAction::triggered, this, [this]() {
    config_.theme = "dark";
    applyTheme(config_.theme);
  });
  connect(lightAction, &QAction::triggered, this, [this]() {
    config_.theme = "light";
    applyTheme(config_.theme);
  });

  auto* helpMenu = menuBar()->addMenu("帮助(Help)");
  helpMenu->addAction("关于", [this] { QMessageBox::information(this, "关于", "Qt RTSP + TCP Client\nPhase1 可用版本"); });
}


void MainWindow::setupUi() {
  setWindowTitle("Qt RTSP + TCP Client Demo");
  setWindowIcon(QIcon(":/icons/app_icon.xpm"));
  resize(1460, 880);
  setMinimumSize(1080, 680);

  auto* root = new QWidget(this);
  auto* rootLayout = new QVBoxLayout(root);

  pages_ = new QStackedWidget(root);

  // 实时页：左侧视频+状态，右侧参数与接收/解析数据
  auto* livePage = new QWidget(pages_);
  auto* liveLayout = new QHBoxLayout(livePage);
  liveLayout->setContentsMargins(8, 8, 8, 8);
  liveLayout->setSpacing(10);

  auto* leftCol = new QWidget(livePage);
  auto* leftLayout = new QVBoxLayout(leftCol);
  leftLayout->setContentsMargins(0, 0, 0, 0);
  leftLayout->setSpacing(10);

  auto* videoBox = new QGroupBox("实时视频", leftCol);
  auto* videoLayout = new QVBoxLayout(videoBox);
  videoWidget_ = new QVideoWidget(videoBox);
  videoWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  videoLayout->addWidget(videoWidget_, 1);

  auto* videoActionRow = new QWidget(videoBox);
  auto* videoActionLayout = new QHBoxLayout(videoActionRow);
  videoActionLayout->setContentsMargins(0, 0, 0, 0);
  screenshotBtn_ = new QPushButton("截图", videoActionRow);
  connect(screenshotBtn_, &QPushButton::clicked, this, &MainWindow::onCaptureScreenshot);
  videoActionLayout->addStretch();
  videoActionLayout->addWidget(screenshotBtn_);
  videoLayout->addWidget(videoActionRow);

  auto* statusBox = new QGroupBox("连接与告警", leftCol);
  auto* statusLayout = new QVBoxLayout(statusBox);

  auto* connRow = new QHBoxLayout();
  connLight_ = new QLabel(statusBox);
  connLight_->setFixedSize(14, 14);
  connLight_->setStyleSheet("background:#ef4444;border-radius:7px;");
  connState_ = new QLabel("stopped", statusBox);
  connLineLabel_ = new QLabel("RTSP: - | TCP: -", statusBox);
  connLineLabel_->setWordWrap(true);
  connRow->addWidget(connLight_);
  connRow->addWidget(connState_);
  connRow->addWidget(new QLabel("|", statusBox));
  connRow->addWidget(connLineLabel_, 1);
  statusLayout->addLayout(connRow);

  auto* alertRow = new QHBoxLayout();
  alertRow->addWidget(new QLabel("告警灯:", statusBox));
  alertLight_ = new QLabel(statusBox);
  alertLight_->setFixedSize(14, 14);
  alertLight_->setStyleSheet("background:#334155;border-radius:7px;");
  alertRow->addWidget(alertLight_);
  alertStateLabel_ = new QLabel("组合告警: idle", statusBox);
  alertRow->addWidget(alertStateLabel_, 1);
  statusLayout->addLayout(alertRow);

  leftLayout->addWidget(videoBox, 5);
  leftLayout->addWidget(statusBox, 2);

  auto* rightCol = new QWidget(livePage);
  auto* rightLayout = new QVBoxLayout(rightCol);
  rightLayout->setContentsMargins(0, 0, 0, 0);
  rightLayout->setSpacing(10);

  auto* cfgGroup = new QGroupBox("参数编辑区", rightCol);
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
  auto* startBtn = new QPushButton("启动", buttonRow);
  auto* stopBtn = new QPushButton("停止", buttonRow);
  buttonLayout->addWidget(startBtn);
  buttonLayout->addWidget(stopBtn);
  connect(startBtn, &QPushButton::clicked, this, &MainWindow::onStartAll);
  connect(stopBtn, &QPushButton::clicked, this, &MainWindow::onStopAll);

  form->addRow("RTSP URL", rtspEdit_);
  form->addRow("TCP Host", tcpHostEdit_);
  form->addRow("TCP Port", tcpPortSpin_);
  form->addRow("重连间隔(ms)", reconnectSpin_);
  form->addRow("录制路径", recordRow);
  form->addRow("录制开关", recordEnabledCheck_);
  form->addRow("控制", buttonRow);

  auto* rawGroup = new QGroupBox("接收数据", rightCol);
  auto* rawLayout = new QVBoxLayout(rawGroup);
  recvDataView_ = new QPlainTextEdit(rawGroup);
  recvDataView_->setReadOnly(true);
  recvDataView_->setMinimumHeight(190);
  rawLayout->addWidget(recvDataView_);

  auto* parsedGroup = new QGroupBox("解析结果", rightCol);
  auto* parsedLayout = new QVBoxLayout(parsedGroup);
  parsedResultView_ = new QPlainTextEdit(parsedGroup);
  parsedResultView_->setReadOnly(true);
  parsedResultView_->setMinimumHeight(170);
  parsedLayout->addWidget(parsedResultView_);

  rightLayout->addWidget(cfgGroup, 3);
  rightLayout->addWidget(rawGroup, 4);
  rightLayout->addWidget(parsedGroup, 3);

  liveLayout->addWidget(leftCol, 3);
  liveLayout->addWidget(rightCol, 2);
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
  settings_->setValue("storage/data_dir", toStoredRelativePath(dataDir_));
  settings_->setValue("storage/logs_dir", toStoredRelativePath(logsDir_));
  settings_->setValue("storage/snapshots_dir", toStoredRelativePath(snapshotsDir_));
  settings_->sync();
  repo_->saveConfig(config_);

  const QString runtimeRecordDir = resolvePath(config_.recordDir, "./recordings");
  QMetaObject::invokeMethod(tcp_, "start", Qt::QueuedConnection, Q_ARG(QString, config_.tcpHost),
                            Q_ARG(quint16, config_.tcpPort), Q_ARG(int, config_.reconnectIntervalMs));
  QMetaObject::invokeMethod(stream_, "start", Qt::QueuedConnection, Q_ARG(QUrl, QUrl(config_.rtspUrl)));
  if (config_.recordEnabled) QMetaObject::invokeMethod(recorder_, "start", Qt::QueuedConnection, Q_ARG(QString, runtimeRecordDir));

  statusBar()->showMessage(QString("运行中 | DB: %1").arg(dbDisplayPath_));
  appendLog("INFO", "app", QString("StatusBar => 运行中 | DB: %1").arg(dbDisplayPath_));
  appendLog("INFO", "app", "Workers started");
}

void MainWindow::onStopAll() {
  QMetaObject::invokeMethod(tcp_, "stop", Qt::QueuedConnection);
  QMetaObject::invokeMethod(stream_, "stop", Qt::QueuedConnection);
  QMetaObject::invokeMethod(recorder_, "stop", Qt::QueuedConnection);
  statusBar()->showMessage(QString("已停止 | DB: %1").arg(dbDisplayPath_));
  appendLog("INFO", "app", QString("StatusBar => 已停止 | DB: %1").arg(dbDisplayPath_));
  appendLog("INFO", "app", "Workers stopped");
}

void MainWindow::onSaveConfig() {
  config_ = collectConfigFromUi();
  config_.windowGeometry = saveGeometry();
  config_.windowState = saveState();
  settings_->setValue("storage/data_dir", toStoredRelativePath(dataDir_));
  settings_->setValue("storage/logs_dir", toStoredRelativePath(logsDir_));
  settings_->setValue("storage/snapshots_dir", toStoredRelativePath(snapshotsDir_));
  settings_->sync();
  repo_->saveConfig(config_);
  applyTheme(config_.theme);
  appendLog("INFO", "app", "Config saved to QSettings + SQLite");
  if (!qEnvironmentVariableIsSet("QT_CLIENT_AUTOTEST")) {
    QMessageBox::information(this, "保存成功", "配置已保存，主题已应用。");
  }
}

void MainWindow::onTelemetry(const demo::client::TelemetryPacket& pkt) {
  lastPkt_ = pkt;

  const QString rawObjects = [&pkt]() {
    QStringList lines;
    int i = 0;
    for (const auto& o : pkt.detection.objects) {
      lines << QString("  [%1] %2 conf=%3 bbox(cx=%4,cy=%5,w=%6,h=%7)")
                   .arg(i++)
                   .arg(o.label)
                   .arg(o.confidence, 0, 'f', 2)
                   .arg(o.bbox.x(), 0, 'f', 3)
                   .arg(o.bbox.y(), 0, 'f', 3)
                   .arg(o.bbox.width(), 0, 'f', 3)
                   .arg(o.bbox.height(), 0, 'f', 3);
    }
    return lines.join("\n");
  }();

  if (recvDataView_) {
    recvDataView_->setPlainText(QString("raw_json: %1\n"
                                        "timestamps(ms): source=%2 sent=%3 recv=%4\n"
                                        "gps_raw(E7): lat=%5 lon=%6 alt_mm=%7 sat=%8\n"
                                        "objects(%9):\n%10")
                                    .arg(pkt.rawJsonLine)
                                    .arg(pkt.detection.sourceTsMs)
                                    .arg(pkt.sentTsMs)
                                    .arg(pkt.recvTsMs)
                                    .arg(pkt.gps.latE7)
                                    .arg(pkt.gps.lonE7)
                                    .arg(pkt.gps.altMm)
                                    .arg(pkt.gps.satellitesVisible)
                                    .arg(pkt.detection.objects.size())
                                    .arg(rawObjects));
  }

  evaluatePersonRodAlert(pkt);

  const QString parsed = QString("是否报警: %1\n"
                                 "当前无人机坐标: lat=%2 (%3) lon=%4 (%5)\n"
                                 "关键检测摘要: label=%6 conf=%7 objects=%8")
                             .arg(alertActive_ ? "是" : "否")
                             .arg(pkt.gps.latE7)
                             .arg(e7ToDegree(pkt.gps.latE7), 0, 'f', 7)
                             .arg(pkt.gps.lonE7)
                             .arg(e7ToDegree(pkt.gps.lonE7), 0, 'f', 7)
                             .arg(pkt.detection.label)
                             .arg(pkt.detection.confidence, 0, 'f', 2)
                             .arg(pkt.detection.objects.size());
  if (parsedResultView_) parsedResultView_->setPlainText(parsed);

  if (parsedResultView_) {
    parsedResultView_->appendPlainText(QString("\nIoU阈值: %1").arg(alertIouThreshold_, 0, 'f', 2));
  }
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
  if (connState_) connState_->setText(QString("连接状态: %1").arg(state));
  if (connLineLabel_) {
    connLineLabel_->setText(QString("RTSP: %1 | TCP: %2:%3").arg(config_.rtspUrl, config_.tcpHost).arg(config_.tcpPort));
  }

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
  const QString dir = snapshotsDir_;
  QDir().mkpath(dir);
  const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
  const QString path = QDir(dir).filePath(QString("shot_%1_%2.jpg").arg(nowMs).arg(reasonTag));

  const auto pix = videoWidget_->grab();
  if (pix.isNull() || !pix.save(path, "JPG")) {
    appendLog("ERROR", "event", "截图失败");
    return {};
  }

  const bool isEvent = alertActive_;
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
  cfg.recordDir = toStoredRelativePath(resolvePath(recordDirEdit_->text().trimmed(), "./recordings"));
  cfg.recordEnabled = recordEnabledCheck_->isChecked();
  if (cfg.theme.isEmpty()) cfg.theme = config_.theme;
  return cfg;
}

void MainWindow::applyConfigToUi(const demo::client::AppConfig& cfg) {
  rtspEdit_->setText(cfg.rtspUrl);
  tcpHostEdit_->setText(cfg.tcpHost);
  tcpPortSpin_->setValue(cfg.tcpPort);
  reconnectSpin_->setValue(cfg.reconnectIntervalMs);
  recordDirEdit_->setText(toStoredRelativePath(resolvePath(cfg.recordDir, "./recordings")));
  recordEnabledCheck_->setChecked(cfg.recordEnabled);
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

  QFile lf(logFilePath_);
  if (lf.open(QIODevice::Append | QIODevice::Text)) {
    QTextStream out(&lf);
    out << QDateTime::fromMSecsSinceEpoch(e.tsMs).toString("yyyy-MM-dd HH:mm:ss.zzz") << " [" << e.level
        << "] [" << e.type << "] " << e.message << "\n";
  }

  refreshLogView();
}


QString MainWindow::resolvePath(const QString& configuredPath, const QString& defaultRelative) const {
  const QString raw = configuredPath.trimmed().isEmpty() ? defaultRelative : configuredPath.trimmed();
  const QFileInfo fi(raw);
  if (fi.isAbsolute()) return QDir::cleanPath(raw);
  return QDir(projectRootDir_).filePath(raw);
}

QString MainWindow::toStoredRelativePath(const QString& path) const {
  const QString rel = QDir(projectRootDir_).relativeFilePath(path);
  if (!rel.startsWith("..")) return rel.startsWith("./") ? rel : QString("./%1").arg(rel);
  return path;
}

void MainWindow::ensureRuntimeDirs() {
  QDir().mkpath(dataDir_);
  QDir().mkpath(logsDir_);
  QDir().mkpath(snapshotsDir_);
}

double MainWindow::bboxIoU(const QRectF& a, const QRectF& b) const {
  const QRectF inter = a.intersected(b);
  if (inter.isEmpty()) return 0.0;
  const double interArea = inter.width() * inter.height();
  const double unionArea = a.width() * a.height() + b.width() * b.height() - interArea;
  return unionArea > 0.0 ? interArea / unionArea : 0.0;
}

void MainWindow::evaluatePersonRodAlert(const demo::client::TelemetryPacket& pkt) {
  QVector<demo::client::DetectionObject> persons;
  QVector<demo::client::DetectionObject> rods;
  for (const auto& obj : pkt.detection.objects) {
    const auto lb = obj.label.trimmed().toLower();
    if (lb == "person") persons.push_back(obj);
    if (lb == "rod") rods.push_back(obj);
  }

  bool hit = false;
  double maxIou = 0.0;
  for (const auto& p : persons) {
    for (const auto& r : rods) {
      const double iou = bboxIoU(p.bbox, r.bbox);
      if (iou > maxIou) maxIou = iou;
      if (iou >= alertIouThreshold_) {
        hit = true;
      }
    }
  }

  alertActive_ = hit;
  if (alertLight_) {
    alertLight_->setStyleSheet(hit ? "background:#f59e0b;border-radius:7px;" : "background:#334155;border-radius:7px;");
  }
  if (alertStateLabel_) {
    alertStateLabel_->setText(hit ? QString("组合告警: ACTIVE (IoU=%1)").arg(maxIou, 0, 'f', 2)
                                  : QString("组合告警: idle (maxIoU=%1)").arg(maxIou, 0, 'f', 2));
  }

  static qint64 lastAlertLogMs = 0;
  const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
  if (hit && nowMs - lastAlertLogMs > 1500) {
    lastAlertLogMs = nowMs;
    QJsonArray arr;
    for (const auto& obj : pkt.detection.objects) {
      QJsonObject jo;
      jo["label"] = obj.label;
      jo["confidence"] = obj.confidence;
      jo["bbox"] = QJsonArray{obj.bbox.x(), obj.bbox.y(), obj.bbox.width(), obj.bbox.height()};
      arr.push_back(jo);
    }
    appendLog("WARN", "event", QString("person+rod告警触发, IoU=%1, objects=%2")
                                  .arg(maxIou, 0, 'f', 2)
                                  .arg(QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact))));
  }
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
