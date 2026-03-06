#include "database/database_service.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace demo::client {

SQLiteDatabaseService::SQLiteDatabaseService(const QString& dbPath, QObject* parent)
    : QObject(parent), connName_(QString("qt_client_conn_%1").arg(reinterpret_cast<quintptr>(this))), dbPath_(dbPath) {}

SQLiteDatabaseService::~SQLiteDatabaseService() {
  if (db_.isOpen()) db_.close();
  if (QSqlDatabase::contains(connName_)) QSqlDatabase::removeDatabase(connName_);
}

bool SQLiteDatabaseService::ensureConnection() {
  if (db_.isValid() && db_.isOpen()) return true;
  QDir().mkpath(QFileInfo(dbPath_).absolutePath());
  db_ = QSqlDatabase::addDatabase("QSQLITE", connName_);
  db_.setDatabaseName(dbPath_);
  return db_.open();
}

bool SQLiteDatabaseService::initialize() {
  if (!ensureConnection()) return false;
  return migrate();
}

bool SQLiteDatabaseService::migrate() {
  QSqlQuery q(db_);
  if (!q.exec("CREATE TABLE IF NOT EXISTS schema_version(version INTEGER NOT NULL);")) return false;

  int version = 0;
  if (q.exec("SELECT version FROM schema_version ORDER BY version DESC LIMIT 1;") && q.next()) {
    version = q.value(0).toInt();
  } else {
    q.exec("DELETE FROM schema_version;");
    q.exec("INSERT INTO schema_version(version) VALUES(0);");
  }

  if (version < 1) {
    if (!q.exec("CREATE TABLE IF NOT EXISTS app_config("
                "id INTEGER PRIMARY KEY CHECK(id=1),"
                "rtsp_url TEXT NOT NULL, tcp_host TEXT NOT NULL, tcp_port INTEGER NOT NULL,"
                "reconnect_ms INTEGER NOT NULL, record_dir TEXT NOT NULL, record_enabled INTEGER NOT NULL,"
                "theme TEXT NOT NULL, window_geometry BLOB, window_state BLOB, updated_at_ms INTEGER NOT NULL);"))
      return false;

    if (!q.exec("CREATE TABLE IF NOT EXISTS telemetry_results("
                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "label TEXT, confidence REAL, source_ts_ms INTEGER, sent_ts_ms INTEGER, recv_ts_ms INTEGER,"
                "gps_time_usec INTEGER, gps_lat_e7 INTEGER, gps_lon_e7 INTEGER, gps_alt_mm INTEGER,"
                "gps_vel_cms INTEGER, gps_cog_cdeg INTEGER, gps_fix_type INTEGER, gps_satellites_visible INTEGER);"))
      return false;

    q.exec("DELETE FROM schema_version;");
    if (!q.exec("INSERT INTO schema_version(version) VALUES(1);") ) return false;
  }
  return true;
}

AppConfig SQLiteDatabaseService::loadConfig() {
  AppConfig cfg;
  if (!ensureConnection()) return cfg;

  QSqlQuery q(db_);
  if (!q.exec("SELECT rtsp_url,tcp_host,tcp_port,reconnect_ms,record_dir,record_enabled,theme,window_geometry,window_state "
              "FROM app_config WHERE id=1 LIMIT 1;")) {
    return cfg;
  }
  if (!q.next()) return cfg;

  cfg.rtspUrl = q.value(0).toString();
  cfg.tcpHost = q.value(1).toString();
  cfg.tcpPort = static_cast<quint16>(q.value(2).toUInt());
  cfg.reconnectIntervalMs = q.value(3).toInt();
  cfg.recordDir = q.value(4).toString();
  cfg.recordEnabled = q.value(5).toInt() != 0;
  cfg.theme = q.value(6).toString();
  cfg.windowGeometry = q.value(7).toByteArray();
  cfg.windowState = q.value(8).toByteArray();
  return cfg;
}

void SQLiteDatabaseService::saveConfigAsync(const demo::client::AppConfig& cfg) {
  if (!ensureConnection()) return;
  QSqlQuery q(db_);
  q.prepare("INSERT OR REPLACE INTO app_config(id,rtsp_url,tcp_host,tcp_port,reconnect_ms,record_dir,record_enabled,theme,window_geometry,window_state,updated_at_ms) "
            "VALUES(1,?,?,?,?,?,?,?,?,?,?);");
  q.addBindValue(cfg.rtspUrl);
  q.addBindValue(cfg.tcpHost);
  q.addBindValue(cfg.tcpPort);
  q.addBindValue(cfg.reconnectIntervalMs);
  q.addBindValue(cfg.recordDir);
  q.addBindValue(cfg.recordEnabled ? 1 : 0);
  q.addBindValue(cfg.theme);
  q.addBindValue(cfg.windowGeometry);
  q.addBindValue(cfg.windowState);
  q.addBindValue(QDateTime::currentMSecsSinceEpoch());
  q.exec();
}

void SQLiteDatabaseService::insertTelemetryAsync(const demo::client::TelemetryPacket& pkt) {
  if (!ensureConnection()) return;
  QSqlQuery q(db_);
  q.prepare("INSERT INTO telemetry_results(label,confidence,source_ts_ms,sent_ts_ms,recv_ts_ms,gps_time_usec,gps_lat_e7,gps_lon_e7,gps_alt_mm,gps_vel_cms,gps_cog_cdeg,gps_fix_type,gps_satellites_visible) "
            "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?);");
  q.addBindValue(pkt.detection.label);
  q.addBindValue(pkt.detection.confidence);
  q.addBindValue(pkt.detection.sourceTsMs);
  q.addBindValue(pkt.sentTsMs);
  q.addBindValue(pkt.recvTsMs);
  q.addBindValue(pkt.gps.timeUsec);
  q.addBindValue(pkt.gps.latE7);
  q.addBindValue(pkt.gps.lonE7);
  q.addBindValue(pkt.gps.altMm);
  q.addBindValue(pkt.gps.velCms);
  q.addBindValue(pkt.gps.cogCdeg);
  q.addBindValue(pkt.gps.fixType);
  q.addBindValue(pkt.gps.satellitesVisible);
  q.exec();
}

} // namespace demo::client
