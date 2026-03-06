#pragma once

#include "core/app_config.h"
#include "core/types.h"

#include <QMainWindow>
#include <QThread>
#include <QVideoFrame>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;
class QVideoWidget;
class QSettings;
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
  void onSaveConfig();
  void onTelemetry(const demo::client::TelemetryPacket& pkt);
  void onFrame(const QVideoFrame& frame, qint64 tsMs);
  void onLog(const QString& msg);
  void onConnectionStateChanged(const QString& state);

private:
  void setupUi();
  void setupMenus();
  demo::client::AppConfig collectConfigFromUi() const;
  void applyConfigToUi(const demo::client::AppConfig& cfg);
  void applyTheme(const QString& theme);
  demo::client::AppConfig loadConfigFromSettings() const;
  void saveConfigToSettings(const demo::client::AppConfig& cfg);

  QLabel* connState_{nullptr};
  QLabel* tsLabel_{nullptr};
  QLabel* detectionLabel_{nullptr};
  QLabel* gpsLabel_{nullptr};

  QLineEdit* rtspEdit_{nullptr};
  QLineEdit* tcpHostEdit_{nullptr};
  QSpinBox* tcpPortSpin_{nullptr};
  QSpinBox* reconnectSpin_{nullptr};
  QLineEdit* recordDirEdit_{nullptr};
  QCheckBox* recordEnabledCheck_{nullptr};
  QComboBox* themeCombo_{nullptr};

  QPlainTextEdit* logs_{nullptr};
  QVideoWidget* videoWidget_{nullptr};

  QThread tcpThread_;
  QThread streamThread_;
  QThread recordThread_;

  demo::client::TcpClientWorker* tcp_{nullptr};
  demo::client::StreamWorker* stream_{nullptr};
  demo::client::RecordWorker* recorder_{nullptr};
  QSettings* settings_{nullptr};

  demo::client::TelemetryPacket lastPkt_{};
  demo::client::AppConfig config_{};
};
