#pragma once

#include "core/app_config.h"
#include "core/types.h"

#include <QObject>
#include <QSqlDatabase>

namespace demo::client {

class IDatabaseService {
public:
  virtual ~IDatabaseService() = default;
  virtual bool initialize() = 0;
  virtual AppConfig loadConfig() = 0;
};

class SQLiteDatabaseService : public QObject, public IDatabaseService {
  Q_OBJECT
public:
  explicit SQLiteDatabaseService(const QString& dbPath, QObject* parent = nullptr);
  ~SQLiteDatabaseService() override;

  bool initialize() override;
  AppConfig loadConfig() override;

public slots:
  void saveConfigAsync(const demo::client::AppConfig& cfg);
  void insertTelemetryAsync(const demo::client::TelemetryPacket& pkt);

private:
  bool ensureConnection();
  bool migrate();
  QString connName_;
  QString dbPath_;
  QSqlDatabase db_;
};

} // namespace demo::client
