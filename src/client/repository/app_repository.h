#pragma once

#include "core/app_config.h"
#include "core/types.h"

#include <QObject>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace demo::client {
class SQLiteDatabaseService;

class IAppRepository {
public:
  virtual ~IAppRepository() = default;
  virtual AppConfig loadConfig() = 0;
  virtual void saveConfig(const AppConfig& cfg) = 0;
  virtual void appendTelemetry(const TelemetryPacket& pkt) = 0;
};

class AppRepository : public QObject, public IAppRepository {
  Q_OBJECT
public:
  AppRepository(SQLiteDatabaseService* db, QSettings* settings, QObject* parent = nullptr);

  AppConfig loadConfig() override;
  void saveConfig(const AppConfig& cfg) override;
  void appendTelemetry(const TelemetryPacket& pkt) override;

private:
  SQLiteDatabaseService* db_{nullptr};
  QSettings* settings_{nullptr};
};

} // namespace demo::client
