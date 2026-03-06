#include "repository/app_repository.h"

#include "database/database_service.h"

#include <QMetaObject>
#include <QSettings>

namespace demo::client {

AppRepository::AppRepository(SQLiteDatabaseService* db, QSettings* settings, QObject* parent)
    : QObject(parent), db_(db), settings_(settings) {}

AppConfig AppRepository::loadConfig() {
  AppConfig cfg;

  if (db_) {
    QMetaObject::invokeMethod(
        db_,
        [&cfg, this]() {
          cfg = db_->loadConfig();
        },
        Qt::BlockingQueuedConnection);
  }

  cfg.rtspUrl = settings_->value("connection/rtsp_url", cfg.rtspUrl).toString();
  cfg.tcpHost = settings_->value("connection/tcp_host", cfg.tcpHost).toString();
  cfg.tcpPort = settings_->value("connection/tcp_port", cfg.tcpPort).toUInt();
  cfg.reconnectIntervalMs = settings_->value("connection/reconnect_ms", cfg.reconnectIntervalMs).toInt();
  cfg.recordDir = settings_->value("record/dir", cfg.recordDir).toString();
  cfg.recordEnabled = settings_->value("record/enabled", cfg.recordEnabled).toBool();
  cfg.theme = settings_->value("view/theme", cfg.theme).toString();
  cfg.alertLowThreshold = settings_->value("alert/low_threshold", cfg.alertLowThreshold).toDouble();
  cfg.alertMidThreshold = settings_->value("alert/mid_threshold", cfg.alertMidThreshold).toDouble();
  cfg.alertHighThreshold = settings_->value("alert/high_threshold", cfg.alertHighThreshold).toDouble();
  cfg.windowGeometry = settings_->value("view/window_geometry", cfg.windowGeometry).toByteArray();
  cfg.windowState = settings_->value("view/window_state", cfg.windowState).toByteArray();

  return cfg;
}

void AppRepository::saveConfig(const AppConfig& cfg) {
  settings_->setValue("connection/rtsp_url", cfg.rtspUrl);
  settings_->setValue("connection/tcp_host", cfg.tcpHost);
  settings_->setValue("connection/tcp_port", cfg.tcpPort);
  settings_->setValue("connection/reconnect_ms", cfg.reconnectIntervalMs);
  settings_->setValue("record/dir", cfg.recordDir);
  settings_->setValue("record/enabled", cfg.recordEnabled);
  settings_->setValue("view/theme", cfg.theme);
  settings_->setValue("alert/low_threshold", cfg.alertLowThreshold);
  settings_->setValue("alert/mid_threshold", cfg.alertMidThreshold);
  settings_->setValue("alert/high_threshold", cfg.alertHighThreshold);
  settings_->setValue("view/window_geometry", cfg.windowGeometry);
  settings_->setValue("view/window_state", cfg.windowState);
  settings_->sync();

  if (db_) {
    QMetaObject::invokeMethod(db_, "saveConfigAsync", Qt::QueuedConnection, Q_ARG(demo::client::AppConfig, cfg));
  }
}

void AppRepository::appendTelemetry(const TelemetryPacket& pkt) {
  if (!db_) return;
  QMetaObject::invokeMethod(db_, "insertTelemetryAsync", Qt::QueuedConnection,
                            Q_ARG(demo::client::TelemetryPacket, pkt));
}

} // namespace demo::client
