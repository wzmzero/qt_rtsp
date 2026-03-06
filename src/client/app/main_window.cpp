#include "app/main_window.h"

#include "database/database_service.h"
#include "media/record_worker.h"
#include "media/stream_worker.h"
#include "network/tcp_client_worker.h"
#include "repository/app_repository.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
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
#include <QTextStream>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QVideoSink>
#include <QVideoWidget>

namespace {
double e7ToDegree(int e7) { return static_cast<double>(e7) / 10000000.0; }

void setLampColor(QLabel* lamp, const QString& color) {
  if (!lamp) return;
  lamp->setStyleSheet(QString("background:%1;border-radius:7px;").arg(color));
}

QWidget* makeStateRow(const QString& title, QLabel** lamp, QLabel** text, QWidget* parent) {
  auto* row = new QWidget(parent);
  auto* layout = new QHBoxLayout(row);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);
  *lamp = new QLabel(row);
  (*lamp)->setFixedSize(14, 14);
  setLampColor(*lamp, "#475569");
  *text = new QLabel(title + "：未连接", row);
  layout->addWidget(*lamp);
  layout->addWidget(*text, 1);
  return row;
}
} // namespace

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
  if (config_.theme.isEmpty()) config_.theme = "dark";
  applyConfigToUi(config_);

  if (!config_.windowGeometry.isEmpty()) restoreGeometry(config_.windowGeometry);
  if (!config_.windowState.isEmpty()) restoreState(config_.windowState);

  refreshConnectionUi();

  statusBar()->showMessage(QString("工程目录: %1 | DB: %2").arg(projectRootDir_, dbDisplayPath_));
  appendLog("INFO", "app", QString("MainWindow initialized, DB display path=%1").arg(dbDisplayPath_));

  if (qEnvironmentVariableIsSet("QT_CLIENT_AUTOTEST")) {
    appendLog("INFO", "app", "AUTOTEST enabled: start->save->stop sequence");
    QTimer::singleShot(400, this, &MainWindow::onStartAll);
    QTimer::singleShot(1500, this, &MainWindow::onSaveConfig);
    QTimer::singleShot(2600, this, &MainWindow::onStopAll);
    QTimer::singleShot(3400, qApp, &QCoreApplication::quit);
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
  fileMenu->addAction("保存配置", QKeySequence::Save, this, &MainWindow::onSaveConfig);
  fileMenu->addSeparator();
  fileMenu->addAction("退出", QKeySequence::Quit, this, &QWidget::close);

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

  darkThemeAction_ = themeMenu->addAction("暗");
  darkThemeAction_->setCheckable(true);
  lightThemeAction_ = themeMenu->addAction("明");
  lightThemeAction_->setCheckable(true);
  themeGroup->addAction(darkThemeAction_);
  themeGroup->addAction(lightThemeAction_);

  connect(darkThemeAction_, &QAction::triggered, this, &MainWindow::onThemeActionTriggered);
  connect(lightThemeAction_, &QAction::triggered, this, &MainWindow::onThemeActionTriggered);

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

  auto* livePage = new QWidget(pages_);
  auto* liveLayout = new QHBoxLayout(livePage);
  liveLayout->setContentsMargins(8, 8, 8, 8);
  liveLayout->setSpacing(10);

  auto* leftCol = new QWidget(livePage);
  auto* leftLayout = new QVBoxLayout(leftCol);
  leftLayout->setContentsMargins(0, 0, 0, 0);

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

  leftLayout->addWidget(videoBox, 1);

  auto* rightCol = new QWidget(livePage);
  auto* rightLayout = new QVBoxLayout(rightCol);
  rightLayout->setContentsMargins(0, 0, 0, 0);
  rightLayout->setSpacing(10);

  auto* connGroup = new QGroupBox("连接状态", rightCol);
  auto* connLayout = new QVBoxLayout(connGroup);
  auto* connRow = new QWidget(connGroup);
  auto* connRowLayout = new QHBoxLayout(connRow);
  connRowLayout->setContentsMargins(0, 0, 0, 0);
  connRowLayout->setSpacing(12);

  rtspLight_ = new QLabel(connRow);
  rtspLight_->setFixedSize(12, 12);
  rtspStateLabel_ = new QLabel("RTSP: 否", connRow);
  connRowLayout->addWidget(rtspLight_);
  connRowLayout->addWidget(rtspStateLabel_);

  tcpLight_ = new QLabel(connRow);
  tcpLight_->setFixedSize(12, 12);
  tcpStateLabel_ = new QLabel("TCP: 否", connRow);
  connRowLayout->addWidget(tcpLight_);
  connRowLayout->addWidget(tcpStateLabel_);

  bindLight_ = new QLabel(connRow);
  bindLight_->setFixedSize(12, 12);
  bindStateLabel_ = new QLabel("绑定: 否", connRow);
  connRowLayout->addWidget(bindLight_);
  connRowLayout->addWidget(bindStateLabel_);
  connRowLayout->addStretch();
  connLayout->addWidget(connRow);

  auto* dataGroup = new QGroupBox("数据接收", rightCol);
  auto* dataLayout = new QVBoxLayout(dataGroup);

  auto* rawGroup = new QGroupBox("原始接收", dataGroup);
  auto* rawLayout = new QVBoxLayout(rawGroup);
  recvDataView_ = new QPlainTextEdit(rawGroup);
  recvDataView_->setReadOnly(true);
  recvDataView_->setMinimumHeight(170);
  rawDetailBtn_ = new QPushButton("查看完整", rawGroup);
  connect(rawDetailBtn_, &QPushButton::clicked, this, &MainWindow::onShowRawDataDetail);
  rawLayout->addWidget(recvDataView_);
  rawLayout->addWidget(rawDetailBtn_, 0, Qt::AlignRight);

  auto* parsedGroup = new QGroupBox("解析结果", dataGroup);
  auto* parsedLayout = new QVBoxLayout(parsedGroup);
  auto* alertRow = new QWidget(parsedGroup);
  auto* alertLayout = new QHBoxLayout(alertRow);
  alertLayout->setContentsMargins(0, 0, 0, 0);
  alertLight_ = new QLabel(alertRow);
  alertLight_->setFixedSize(14, 14);
  setLampColor(alertLight_, "#334155");
  alertStateLabel_ = new QLabel("person+rod/IoU 告警：idle", alertRow);
  alertLayout->addWidget(alertLight_);
  alertLayout->addWidget(alertStateLabel_, 1);

  gpsLabel_ = new QLabel("经纬度: --", parsedGroup);
  gpsLabel_->setStyleSheet("font-size:14px;font-weight:700;");
  detectionSummaryLabel_ = new QLabel("检测摘要: --", parsedGroup);
  detectionSummaryLabel_->setWordWrap(true);

  parsedResultView_ = new QPlainTextEdit(parsedGroup);
  parsedResultView_->setReadOnly(true);
  parsedResultView_->setMinimumHeight(130);

  parsedLayout->addWidget(alertRow);
  parsedLayout->addWidget(gpsLabel_);
  parsedLayout->addWidget(detectionSummaryLabel_);
  parsedLayout->addWidget(parsedResultView_);

  dataLayout->addWidget(rawGroup, 1);
  dataLayout->addWidget(parsedGroup, 1);

  auto* cfgGroup = new QGroupBox("参数编辑区", rightCol);
  auto* form = new QFormLayout(cfgGroup);
  rtspEdit_ = new QLineEdit(cfgGroup);
  tcpHostEdit_ = new QLineEdit(cfgGroup);
  tcpPortSpin_ = new QSpinBox(cfgGroup);
  tcpPortSpin_->setRange(1, 65535);
  reconnectSpin_ = new QSpinBox(cfgGroup);
  reconnectSpin_->setRange(500, 60000);
  reconnectSpin_->setSingleStep(500);
  alertLowSpin_ = new QDoubleSpinBox(cfgGroup);
  alertMidSpin_ = new QDoubleSpinBox(cfgGroup);
  alertHighSpin_ = new QDoubleSpinBox(cfgGroup);
  for (auto* spin : {alertLowSpin_, alertMidSpin_, alertHighSpin_}) {
    spin->setRange(0.0, 1.0);
    spin->setSingleStep(0.01);
    spin->setDecimals(2);
  }
  alertLowSpin_->setValue(alertLowThreshold_);
  alertMidSpin_->setValue(alertMidThreshold_);
  alertHighSpin_->setValue(alertHighThreshold_);
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
  form->addRow("告警低阈值", alertLowSpin_);
  form->addRow("告警中阈值", alertMidSpin_);
  form->addRow("告警高阈值", alertHighSpin_);
  form->addRow("录制路径", recordRow);
  form->addRow("录制开关", recordEnabledCheck_);
  form->addRow("控制", buttonRow);

  rightLayout->addWidget(connGroup, 2);
  rightLayout->addWidget(dataGroup, 5);
  rightLayout->addWidget(cfgGroup, 3);

  liveLayout->addWidget(leftCol, 5);
  liveLayout->addWidget(rightCol, 3);
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

  rtspConnected_ = false;
  tcpConnected_ = false;
  responseBound_ = false;
  refreshConnectionUi();

  const QString runtimeRecordDir = resolvePath(config_.recordDir, "./recordings");
  QMetaObject::invokeMethod(tcp_, "start", Qt::QueuedConnection, Q_ARG(QString, config_.tcpHost),
                            Q_ARG(quint16, config_.tcpPort), Q_ARG(int, config_.reconnectIntervalMs));
  QMetaObject::invokeMethod(stream_, "start", Qt::QueuedConnection, Q_ARG(QUrl, QUrl(config_.rtspUrl)));
  if (config_.recordEnabled) QMetaObject::invokeMethod(recorder_, "start", Qt::QueuedConnection, Q_ARG(QString, runtimeRecordDir));

  statusBar()->showMessage(QString("运行中 | DB: %1").arg(dbDisplayPath_));
  appendLog("INFO", "app", "Workers started");
}

void MainWindow::onStopAll() {
  QMetaObject::invokeMethod(tcp_, "stop", Qt::QueuedConnection);
  QMetaObject::invokeMethod(stream_, "stop", Qt::QueuedConnection);
  QMetaObject::invokeMethod(recorder_, "stop", Qt::QueuedConnection);
  rtspConnected_ = false;
  tcpConnected_ = false;
  responseBound_ = false;
  connBlinkTimer_->stop();
  refreshConnectionUi();
  statusBar()->showMessage(QString("已停止 | DB: %1").arg(dbDisplayPath_));
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
  appendLog("INFO", "app", QString("Config saved, theme=%1").arg(config_.theme));
  if (!qEnvironmentVariableIsSet("QT_CLIENT_AUTOTEST")) {
    QMessageBox::information(this, "保存成功", "配置已保存，主题已应用。");
  }
}

void MainWindow::onTelemetry(const demo::client::TelemetryPacket& pkt) {
  lastPkt_ = pkt;
  lastTelemetryTsMs_ = QDateTime::currentMSecsSinceEpoch();
  tcpConnected_ = true;
  responseBound_ = (lastFrameTsMs_ > 0 && qAbs(lastFrameTsMs_ - lastTelemetryTsMs_) < 5000);

  QString rawLine = pkt.rawJsonLine;
  if (rawLine.size() > 220) rawLine = rawLine.left(220) + " ...";
  rawHistory_.prepend(QString("[%1] %2")
                          .arg(QDateTime::fromMSecsSinceEpoch(lastTelemetryTsMs_).toString("HH:mm:ss"), rawLine));
  while (rawHistory_.size() > 200) rawHistory_.removeLast();
  recvDataView_->setPlainText(rawHistory_.mid(0, 20).join("\n"));

  evaluatePersonRodAlert(pkt);
  refreshParsedUi(pkt);
  refreshConnectionUi();
  repo_->appendTelemetry(pkt);
}

void MainWindow::onFrame(const QVideoFrame& frame, qint64 tsMs) {
  if (videoWidget_ && videoWidget_->videoSink()) {
    videoWidget_->videoSink()->setVideoFrame(frame);
  }
  rtspConnected_ = true;
  lastFrameTsMs_ = tsMs > 0 ? tsMs : QDateTime::currentMSecsSinceEpoch();
  responseBound_ = (lastTelemetryTsMs_ > 0 && qAbs(lastFrameTsMs_ - lastTelemetryTsMs_) < 5000);
  refreshConnectionUi();

  if (!config_.recordEnabled) return;
  demo::client::RecordItem item;
  item.frameTsMs = tsMs;
  item.frame = frame;
  item.telemetry = lastPkt_;
  QMetaObject::invokeMethod(recorder_, "enqueue", Qt::QueuedConnection, Q_ARG(demo::client::RecordItem, item));
}

void MainWindow::onConnectionStateChanged(const QString& state) {
  connStateValue_ = state;
  const bool connected = state.contains("connected", Qt::CaseInsensitive) || state.contains("running", Qt::CaseInsensitive);
  tcpConnected_ = connected;
  if (connected) {
    connBlinkTimer_->start();
  } else {
    connBlinkTimer_->stop();
    connBlinkOn_ = false;
  }
  refreshConnectionUi();
}

void MainWindow::onThemeActionTriggered() {
  if (lightThemeAction_ && lightThemeAction_->isChecked()) {
    config_.theme = "light";
  } else {
    config_.theme = "dark";
  }
  applyTheme(config_.theme);
}

void MainWindow::onShowRawDataDetail() {
  auto* dlg = new QDialog(this);
  dlg->setWindowTitle("完整原始接收数据");
  dlg->resize(860, 560);
  auto* v = new QVBoxLayout(dlg);
  auto* viewer = new QPlainTextEdit(dlg);
  viewer->setReadOnly(true);
  viewer->setPlainText(rawHistory_.join("\n"));
  v->addWidget(viewer);
  auto* btns = new QDialogButtonBox(QDialogButtonBox::Close, dlg);
  connect(btns, &QDialogButtonBox::rejected, dlg, &QDialog::reject);
  v->addWidget(btns);
  dlg->exec();
}

void MainWindow::blinkConnectionIndicator() {
  connBlinkOn_ = !connBlinkOn_;
  refreshConnectionUi();
}

QString MainWindow::buildDetectionSummary(const demo::client::TelemetryPacket& pkt, int topN) const {
  QStringList items;
  for (const auto& o : pkt.detection.objects) {
    items << QString("%1:%2").arg(o.label).arg(o.confidence, 0, 'f', 2);
  }
  const int shown = qMin(topN, items.size());
  QString summary = shown > 0 ? items.mid(0, shown).join(" | ") : QString("无目标");
  if (items.size() > shown) {
    summary += QString(" | 其余%1个...").arg(items.size() - shown);
  }
  return summary;
}

void MainWindow::refreshConnectionUi() {
  setLampColor(rtspLight_, rtspConnected_ ? "#22c55e" : "#ef4444");
  setLampColor(bindLight_, responseBound_ ? "#22c55e" : "#ef4444");

  if (tcpConnected_) {
    setLampColor(tcpLight_, connBlinkOn_ ? "#22c55e" : "#166534");
  } else {
    setLampColor(tcpLight_, "#ef4444");
  }

  if (rtspStateLabel_) rtspStateLabel_->setText(QString("RTSP: %1").arg(rtspConnected_ ? "是" : "否"));
  if (tcpStateLabel_) tcpStateLabel_->setText(QString("TCP: %1").arg(tcpConnected_ ? "是" : "否"));
  if (bindStateLabel_) bindStateLabel_->setText(QString("绑定: %1").arg(responseBound_ ? "是" : "否"));
}

void MainWindow::refreshParsedUi(const demo::client::TelemetryPacket& pkt) {
  if (gpsLabel_) {
    gpsLabel_->setText(
        QString("经纬度: %1°, %2°")
            .arg(e7ToDegree(pkt.gps.latE7), 0, 'f', 7)
            .arg(e7ToDegree(pkt.gps.lonE7), 0, 'f', 7));
  }
  if (detectionSummaryLabel_) {
    detectionSummaryLabel_->setText(QString("检测摘要: %1").arg(buildDetectionSummary(pkt)));
  }

  double personConf = 0.0;
  double rodConf = 0.0;
  for (const auto& o : pkt.detection.objects) {
    const auto lb = o.label.trimmed().toLower();
    if (lb == "person") personConf = qMax(personConf, o.confidence);
    if (lb == "rod") rodConf = qMax(rodConf, o.confidence);
  }

  QString level = "无告警";
  if (personConf >= alertHighThreshold_ && rodConf >= alertHighThreshold_) level = "高告警(红色)";
  else if (personConf >= alertMidThreshold_ && rodConf >= alertMidThreshold_) level = "中告警(深黄)";
  else if (personConf >= alertLowThreshold_ && rodConf >= alertLowThreshold_) level = "低告警(浅黄)";

  QStringList details;
  details << QString("告警级别: %1").arg(level);
  details << QString("触发条件: person_conf=%1, rod_conf=%2")
                 .arg(personConf, 0, 'f', 2)
                 .arg(rodConf, 0, 'f', 2);
  details << QString("阈值 low/mid/high: %1 / %2 / %3")
                 .arg(alertLowThreshold_, 0, 'f', 2)
                 .arg(alertMidThreshold_, 0, 'f', 2)
                 .arg(alertHighThreshold_, 0, 'f', 2);
  const auto t = QDateTime::fromMSecsSinceEpoch(pkt.recvTsMs).toString("HH:mm:ss");
  details << QString("解析时间: %1").arg(t);
  details << QString("objects: %1").arg(pkt.detection.objects.size());
  parsedResultView_->setPlainText(details.join("\n"));
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
  cfg.theme = config_.theme.isEmpty() ? "dark" : config_.theme;
  cfg.alertLowThreshold = alertLowSpin_ ? alertLowSpin_->value() : alertLowThreshold_;
  cfg.alertMidThreshold = alertMidSpin_ ? alertMidSpin_->value() : alertMidThreshold_;
  cfg.alertHighThreshold = alertHighSpin_ ? alertHighSpin_->value() : alertHighThreshold_;
  return cfg;
}

void MainWindow::applyConfigToUi(const demo::client::AppConfig& cfg) {
  rtspEdit_->setText(cfg.rtspUrl);
  tcpHostEdit_->setText(cfg.tcpHost);
  tcpPortSpin_->setValue(cfg.tcpPort);
  reconnectSpin_->setValue(cfg.reconnectIntervalMs);
  recordDirEdit_->setText(toStoredRelativePath(resolvePath(cfg.recordDir, "./recordings")));
  recordEnabledCheck_->setChecked(cfg.recordEnabled);
  config_.theme = cfg.theme.isEmpty() ? "dark" : cfg.theme;
  alertLowThreshold_ = cfg.alertLowThreshold;
  alertMidThreshold_ = cfg.alertMidThreshold;
  alertHighThreshold_ = cfg.alertHighThreshold;
  if (alertLowSpin_) alertLowSpin_->setValue(alertLowThreshold_);
  if (alertMidSpin_) alertMidSpin_->setValue(alertMidThreshold_);
  if (alertHighSpin_) alertHighSpin_->setValue(alertHighThreshold_);
  if (darkThemeAction_ && lightThemeAction_) {
    darkThemeAction_->setChecked(config_.theme != "light");
    lightThemeAction_->setChecked(config_.theme == "light");
  }
  applyTheme(config_.theme);
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
  double personConf = 0.0;
  double rodConf = 0.0;
  for (const auto& obj : pkt.detection.objects) {
    const auto lb = obj.label.trimmed().toLower();
    if (lb == "person") personConf = qMax(personConf, obj.confidence);
    if (lb == "rod") rodConf = qMax(rodConf, obj.confidence);
  }

  const bool lowHit = personConf >= alertLowThreshold_ && rodConf >= alertLowThreshold_;
  const bool midHit = personConf >= alertMidThreshold_ && rodConf >= alertMidThreshold_;
  const bool highHit = personConf >= alertHighThreshold_ && rodConf >= alertHighThreshold_;

  alertActive_ = lowHit;
  QString levelText = "idle";
  if (highHit) {
    setLampColor(alertLight_, "#ef4444");
    levelText = "high";
    if (alertStateLabel_) alertStateLabel_->setText(QString("告警：高(红) person=%1 rod=%2").arg(personConf, 0, 'f', 2).arg(rodConf, 0, 'f', 2));
  } else if (midHit) {
    setLampColor(alertLight_, "#f59e0b");
    levelText = "mid";
    if (alertStateLabel_) alertStateLabel_->setText(QString("告警：中(深黄) person=%1 rod=%2").arg(personConf, 0, 'f', 2).arg(rodConf, 0, 'f', 2));
  } else if (lowHit) {
    setLampColor(alertLight_, "#fde68a");
    levelText = "low";
    if (alertStateLabel_) alertStateLabel_->setText(QString("告警：低(浅黄) person=%1 rod=%2").arg(personConf, 0, 'f', 2).arg(rodConf, 0, 'f', 2));
  } else {
    setLampColor(alertLight_, "#334155");
    if (alertStateLabel_) alertStateLabel_->setText("告警：idle（需同帧出现person与rod）");
  }

  static QString lastLevel;
  if (lastLevel != levelText) {
    appendLog("INFO", "event", QString("告警级别切换: %1 (person=%2, rod=%3)").arg(levelText).arg(personConf, 0, 'f', 2).arg(rodConf, 0, 'f', 2));
    lastLevel = levelText;
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
