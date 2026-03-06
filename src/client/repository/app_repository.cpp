#include "repository/app_repository.h"

#include "database/database_service.h"

#include <QMetaObject>

namespace demo::client {

AppRepository::AppRepository(SQLiteDatabaseService* db, QObject* parent)
    : QObject(parent), db_(db) {}

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

  return cfg;
}


void AppRepository::saveConfig(const AppConfig& cfg) {
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
