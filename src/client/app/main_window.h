#pragma once

#include "core/app_config.h"
#include "core/types.h"

#include <QMainWindow>
#include <QRectF>
#include <QThread>
#include <QVideoFrame>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QDateTimeEdit;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QTableWidget;
class QVideoWidget;
class QSettings;
class QTimer;
QT_END_NAMESPACE

namespace demo::client {
class TcpClientWorker;
class StreamWorker;
class RecordWorker;
class SQLiteDatabaseService;
class AppRepository;
}

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override;

private slots:
  void onStartAll();
  void onStopAll();
  void onSaveConfig();
  void onTelemetry(const demo::client::TelemetryPacket& pkt);
  void onFrame(const QVideoFrame& frame, qint64 tsMs);
  void onConnectionStateChanged(const QString& state);
  void onCaptureScreenshot();
  void onRefreshEvents();
  void onLogFilterChanged();
  void blinkConnectionIndicator();

private:
  void setupUi();
  void setupMenus();
  demo::client::AppConfig collectConfigFromUi() const;
  void applyConfigToUi(const demo::client::AppConfig& cfg);
  void applyTheme(const QString& theme);
  void appendLog(const QString& level, const QString& type, const QString& msg);
  void refreshLogView();
  QString saveScreenshotWithMetadata(const QString& reasonTag);
  QString resolvePath(const QString& configuredPath, const QString& defaultRelative) const;
  QString toStoredRelativePath(const QString& path) const;
  void ensureRuntimeDirs();
  void evaluatePersonRodAlert(const demo::client::TelemetryPacket& pkt);
  double bboxIoU(const QRectF& a, const QRectF& b) const;

  struct LogEntry {
    qint64 tsMs{0};
    QString level;
    QString type;
    QString message;
  };

  QStackedWidget* pages_{nullptr};

  QLabel* connState_{nullptr};
  QLabel* connLineLabel_{nullptr};
  QLabel* connLight_{nullptr};
  QLabel* alertLight_{nullptr};
  QLabel* alertStateLabel_{nullptr};
  QPlainTextEdit* recvDataView_{nullptr};
  QPlainTextEdit* parsedResultView_{nullptr};

  QLineEdit* rtspEdit_{nullptr};
  QLineEdit* tcpHostEdit_{nullptr};
  QSpinBox* tcpPortSpin_{nullptr};
  QSpinBox* reconnectSpin_{nullptr};
  QLineEdit* recordDirEdit_{nullptr};
  QCheckBox* recordEnabledCheck_{nullptr};
  QComboBox* themeCombo_{nullptr};

  QComboBox* logLevelFilter_{nullptr};
  QComboBox* logTypeFilter_{nullptr};
  QPlainTextEdit* logs_{nullptr};

  QLineEdit* eventLabelFilter_{nullptr};
  QDateTimeEdit* eventFrom_{nullptr};
  QDateTimeEdit* eventTo_{nullptr};
  QTableWidget* eventTable_{nullptr};

  QVideoWidget* videoWidget_{nullptr};
  QPushButton* screenshotBtn_{nullptr};
  QTimer* connBlinkTimer_{nullptr};
  bool connBlinkOn_{false};
  QString connStateValue_;

  QList<LogEntry> logEntries_;

  QThread tcpThread_;
  QThread streamThread_;
  QThread recordThread_;
  QThread dbThread_;

  demo::client::TcpClientWorker* tcp_{nullptr};
  demo::client::StreamWorker* stream_{nullptr};
  demo::client::RecordWorker* recorder_{nullptr};
  demo::client::SQLiteDatabaseService* db_{nullptr};
  demo::client::AppRepository* repo_{nullptr};
  QSettings* settings_{nullptr};

  demo::client::TelemetryPacket lastPkt_{};
  demo::client::AppConfig config_{};
  QString projectRootDir_;
  QString dataDir_;
  QString logsDir_;
  QString snapshotsDir_;
  QString dbPath_;
  QString dbDisplayPath_;
  QString logFilePath_;
  bool alertActive_{false};
  double alertIouThreshold_{0.10};
};
