#include "database/database_service.h"

#include <QDateTime>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QThread>
#include <QVariant>

namespace demo::client {

SQLiteDatabaseService::SQLiteDatabaseService(const QString& dbPath, QObject* parent)
    : QObject(parent), dbPath_(dbPath) {}

SQLiteDatabaseService::~SQLiteDatabaseService() {
  if (db_.isOpen()) db_.close();
  const auto connName = db_.connectionName();
  db_ = QSqlDatabase();
  if (!connName.isEmpty() && QSqlDatabase::contains(connName)) {
    QSqlDatabase::removeDatabase(connName);
  }
}

QString SQLiteDatabaseService::connectionNameForCurrentThread() const {
  return QString("qt_client_conn_%1").arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));
}

bool SQLiteDatabaseService::ensureConnection() {
  if (db_.isValid() && db_.isOpen()) return true;

  QDir().mkpath(QFileInfo(dbPath_).absolutePath());

  const auto connName = connectionNameForCurrentThread();
  if (QSqlDatabase::contains(connName)) {
    db_ = QSqlDatabase::database(connName);
  } else {
    db_ = QSqlDatabase::addDatabase("QSQLITE", connName);
    db_.setDatabaseName(dbPath_);
  }

  if (!db_.isOpen() && !db_.open()) {
    qWarning() << "SQLite open failed:" << db_.lastError().text();
    return false;
  }
  return true;
}


bool SQLiteDatabaseService::initialize() {
  if (!ensureConnection()) return false;
  return migrate();
}

bool SQLiteDatabaseService::migrate() {
  QSqlQuery q(db_);

  if (!q.exec("CREATE TABLE IF NOT EXISTS schema_version(version INTEGER NOT NULL);")) {
    qWarning() << "Create schema_version bootstrap failed:" << q.lastError().text();
    return false;
  }

  int version = 0;
  if (q.exec("SELECT MAX(version) FROM schema_version;") && q.next() && !q.value(0).isNull()) {
    version = q.value(0).toInt();
  }

  // normalize schema_version to (version, applied_at_ms)
  int schemaVersionCols = 0;
  if (q.exec("PRAGMA table_info(schema_version);") ) {
    while (q.next()) ++schemaVersionCols;
  }
  if (schemaVersionCols < 2) {
    q.exec("DROP TABLE IF EXISTS schema_version;");
    if (!q.exec("CREATE TABLE schema_version(version INTEGER NOT NULL, applied_at_ms INTEGER NOT NULL);")) {
      qWarning() << "Create schema_version normalized failed:" << q.lastError().text();
      return false;
    }
    if (!q.exec(QString("INSERT INTO schema_version(version,applied_at_ms) VALUES(%1,%2);")
                    .arg(version)
                    .arg(QDateTime::currentMSecsSinceEpoch()))) {
      qWarning() << "Seed schema_version failed:" << q.lastError().text();
      return false;
    }
  }

  // v3 functional schema
  if (!q.exec("CREATE TABLE IF NOT EXISTS app_config("
              "id INTEGER PRIMARY KEY CHECK(id=1),"
              "rtsp_url TEXT NOT NULL, tcp_host TEXT NOT NULL, tcp_port INTEGER NOT NULL,"
              "reconnect_ms INTEGER NOT NULL, record_dir TEXT NOT NULL, record_enabled INTEGER NOT NULL,"
              "theme TEXT NOT NULL, window_geometry BLOB, window_state BLOB, updated_at_ms INTEGER NOT NULL);")) {
    qWarning() << "Create app_config failed:" << q.lastError().text();
    return false;
  }

  if (!q.exec("CREATE TABLE IF NOT EXISTS telemetry_results("
              "id INTEGER PRIMARY KEY AUTOINCREMENT,"
              "recv_ts_ms INTEGER NOT NULL, sent_ts_ms INTEGER NOT NULL, source_ts_ms INTEGER NOT NULL,"
              "label TEXT, confidence REAL, detection_objects_json TEXT);")) {
    qWarning() << "Create telemetry_results failed:" << q.lastError().text();
    return false;
  }

  if (!q.exec("CREATE TABLE IF NOT EXISTS gps_samples("
              "id INTEGER PRIMARY KEY AUTOINCREMENT,"
              "telemetry_id INTEGER,"
              "sample_ts_ms INTEGER NOT NULL,"
              "time_usec INTEGER, lat_e7 INTEGER, lon_e7 INTEGER, alt_mm INTEGER,"
              "vel_cms INTEGER, cog_cdeg INTEGER, fix_type INTEGER, satellites_visible INTEGER,"
              "lat_deg REAL, lon_deg REAL,"
              "FOREIGN KEY(telemetry_id) REFERENCES telemetry_results(id));")) {
    qWarning() << "Create gps_samples failed:" << q.lastError().text();
    return false;
  }

  if (!q.exec("CREATE TABLE IF NOT EXISTS snapshot_events("
              "id INTEGER PRIMARY KEY AUTOINCREMENT,"
              "event_ts_ms INTEGER NOT NULL,"
              "telemetry_id INTEGER,"
              "screenshot_path TEXT NOT NULL,"
              "reason_tag TEXT NOT NULL,"
              "label TEXT, confidence REAL, is_target_event INTEGER NOT NULL DEFAULT 0,"
              "FOREIGN KEY(telemetry_id) REFERENCES telemetry_results(id));")) {
    qWarning() << "Create snapshot_events failed:" << q.lastError().text();
    return false;
  }

  if (!q.exec("CREATE TABLE IF NOT EXISTS app_logs("
              "id INTEGER PRIMARY KEY AUTOINCREMENT,"
              "ts_ms INTEGER NOT NULL,"
              "level TEXT NOT NULL,"
              "type TEXT NOT NULL,"
              "message TEXT NOT NULL);")) {
    qWarning() << "Create app_logs failed:" << q.lastError().text();
    return false;
  }

  if (!q.exec("CREATE TABLE IF NOT EXISTS playback_index("
              "id INTEGER PRIMARY KEY AUTOINCREMENT,"
              "frame_ts_ms INTEGER NOT NULL,"
              "wall_ts_ms INTEGER NOT NULL,"
              "meta_path TEXT NOT NULL,"
              "latency_ms INTEGER NOT NULL,"
              "label TEXT, confidence REAL);")) {
    qWarning() << "Create playback_index failed:" << q.lastError().text();
    return false;
  }

  q.exec("CREATE INDEX IF NOT EXISTS idx_telemetry_ts ON telemetry_results(recv_ts_ms);");
  q.exec("CREATE INDEX IF NOT EXISTS idx_gps_telemetry ON gps_samples(telemetry_id);");
  q.exec("CREATE INDEX IF NOT EXISTS idx_snapshot_filter ON snapshot_events(is_target_event, event_ts_ms, label);");
  q.exec("CREATE INDEX IF NOT EXISTS idx_logs_filter ON app_logs(level, type, ts_ms);");

  if (version < 3) {
    const bool hasLegacy = q.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='telemetry_objects';") && q.next();
    if (hasLegacy) {
      q.exec("INSERT INTO telemetry_results(recv_ts_ms,sent_ts_ms,source_ts_ms,label,confidence) "
             "SELECT recv_ts_ms,sent_ts_ms,source_ts_ms,label,confidence FROM telemetry_objects;");

      q.exec("INSERT INTO gps_samples(telemetry_id,sample_ts_ms,time_usec,lat_e7,lon_e7,alt_mm,vel_cms,cog_cdeg,fix_type,satellites_visible,lat_deg,lon_deg) "
             "SELECT tr.id,tobj.recv_ts_ms,tobj.gps_time_usec,tobj.gps_lat_e7,tobj.gps_lon_e7,tobj.gps_alt_mm,tobj.gps_vel_cms,tobj.gps_cog_cdeg,tobj.gps_fix_type,tobj.gps_satellites_visible,"
             "(CAST(tobj.gps_lat_e7 AS REAL)/10000000.0),(CAST(tobj.gps_lon_e7 AS REAL)/10000000.0) "
             "FROM telemetry_objects tobj "
             "LEFT JOIN telemetry_results tr ON tr.recv_ts_ms=tobj.recv_ts_ms AND tr.sent_ts_ms=tobj.sent_ts_ms AND tr.source_ts_ms=tobj.source_ts_ms "
             "AND IFNULL(tr.label,'')=IFNULL(tobj.label,'') LIMIT -1;");

      q.exec("INSERT INTO snapshot_events(event_ts_ms,telemetry_id,screenshot_path,reason_tag,label,confidence,is_target_event) "
             "SELECT tobj.recv_ts_ms,tr.id,tobj.screenshot_path,'legacy',tobj.label,tobj.confidence,tobj.is_target_event "
             "FROM telemetry_objects tobj "
             "LEFT JOIN telemetry_results tr ON tr.recv_ts_ms=tobj.recv_ts_ms AND tr.sent_ts_ms=tobj.sent_ts_ms AND tr.source_ts_ms=tobj.source_ts_ms "
             "AND IFNULL(tr.label,'')=IFNULL(tobj.label,'') "
             "WHERE tobj.screenshot_path IS NOT NULL AND tobj.screenshot_path != '';");

      q.exec("DROP TABLE IF EXISTS telemetry_objects;");
    }

    q.exec("DELETE FROM schema_version;");
    if (!q.exec(QString("INSERT INTO schema_version(version,applied_at_ms) VALUES(3,%1);")
                    .arg(QDateTime::currentMSecsSinceEpoch()))) {
      qWarning() << "Update schema_version to v3 failed:" << q.lastError().text();
      return false;
    }
  }

  // ensure v4 columns exist
  bool hasDetObjCol = false;
  if (q.exec("PRAGMA table_info(telemetry_results);") ) {
    while (q.next()) {
      if (q.value(1).toString() == "detection_objects_json") {
        hasDetObjCol = true;
        break;
      }
    }
  }
  if (!hasDetObjCol) {
    q.exec("ALTER TABLE telemetry_results ADD COLUMN detection_objects_json TEXT;");
  }

  if (version < 4) {
    q.exec("DELETE FROM schema_version;");
    if (!q.exec(QString("INSERT INTO schema_version(version,applied_at_ms) VALUES(4,%1);")
                    .arg(QDateTime::currentMSecsSinceEpoch()))) {
      qWarning() << "Update schema_version to v4 failed:" << q.lastError().text();
      return false;
    }
  }

  return true;
}

AppConfig SQLiteDatabaseService::loadConfig() {
  AppConfig cfg;
  if (!ensureConnection()) return cfg;

  QSqlQuery q(db_);
  if (!q.exec("SELECT rtsp_url,tcp_host,tcp_port,reconnect_ms,record_dir,record_enabled,theme,window_geometry,window_state "
              "FROM app_config WHERE id=1 LIMIT 1;")) {
    qWarning() << "Load config failed:" << q.lastError().text();
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
  if (!q.exec()) {
    qWarning() << "Save config failed:" << q.lastError().text();
  }
}

void SQLiteDatabaseService::insertTelemetryAsync(const demo::client::TelemetryPacket& pkt) {
  if (!ensureConnection()) return;

  QSqlQuery telemetryQ(db_);
  telemetryQ.prepare("INSERT INTO telemetry_results(recv_ts_ms,sent_ts_ms,source_ts_ms,label,confidence,detection_objects_json) VALUES(?,?,?,?,?,?);");
  telemetryQ.addBindValue(pkt.recvTsMs);
  telemetryQ.addBindValue(pkt.sentTsMs);
  telemetryQ.addBindValue(pkt.detection.sourceTsMs);
  telemetryQ.addBindValue(pkt.detection.label);
  telemetryQ.addBindValue(pkt.detection.confidence);
  QJsonArray objects;
  for (const auto& obj : pkt.detection.objects) {
    QJsonObject jo;
    jo["label"] = obj.label;
    jo["confidence"] = obj.confidence;
    jo["bbox"] = QJsonArray{obj.bbox.x(), obj.bbox.y(), obj.bbox.width(), obj.bbox.height()};
    objects.push_back(jo);
  }
  telemetryQ.addBindValue(QString::fromUtf8(QJsonDocument(objects).toJson(QJsonDocument::Compact)));
  if (!telemetryQ.exec()) {
    qWarning() << "Insert telemetry_results failed:" << telemetryQ.lastError().text();
    return;
  }

  const auto telemetryId = telemetryQ.lastInsertId().toLongLong();

  QSqlQuery gpsQ(db_);
  gpsQ.prepare("INSERT INTO gps_samples(telemetry_id,sample_ts_ms,time_usec,lat_e7,lon_e7,alt_mm,vel_cms,cog_cdeg,fix_type,satellites_visible,lat_deg,lon_deg) "
               "VALUES(?,?,?,?,?,?,?,?,?,?,?,?);");
  gpsQ.addBindValue(telemetryId);
  gpsQ.addBindValue(pkt.recvTsMs);
  gpsQ.addBindValue(pkt.gps.timeUsec);
  gpsQ.addBindValue(pkt.gps.latE7);
  gpsQ.addBindValue(pkt.gps.lonE7);
  gpsQ.addBindValue(pkt.gps.altMm);
  gpsQ.addBindValue(pkt.gps.velCms);
  gpsQ.addBindValue(pkt.gps.cogCdeg);
  gpsQ.addBindValue(pkt.gps.fixType);
  gpsQ.addBindValue(pkt.gps.satellitesVisible);
  gpsQ.addBindValue(static_cast<double>(pkt.gps.latE7) / 10000000.0);
  gpsQ.addBindValue(static_cast<double>(pkt.gps.lonE7) / 10000000.0);
  if (!gpsQ.exec()) {
    qWarning() << "Insert gps_samples failed:" << gpsQ.lastError().text();
  }
}

void SQLiteDatabaseService::insertSnapshotEventAsync(const demo::client::TelemetryPacket& pkt,
                                                     const QString& screenshotPath,
                                                     const QString& reasonTag,
                                                     bool isTargetEvent) {
  if (!ensureConnection()) return;
  if (screenshotPath.trimmed().isEmpty()) return;

  QSqlQuery telemetryQ(db_);
  telemetryQ.prepare("INSERT INTO telemetry_results(recv_ts_ms,sent_ts_ms,source_ts_ms,label,confidence,detection_objects_json) VALUES(?,?,?,?,?,?);");
  telemetryQ.addBindValue(pkt.recvTsMs);
  telemetryQ.addBindValue(pkt.sentTsMs);
  telemetryQ.addBindValue(pkt.detection.sourceTsMs);
  telemetryQ.addBindValue(pkt.detection.label);
  telemetryQ.addBindValue(pkt.detection.confidence);
  QJsonArray objects;
  for (const auto& obj : pkt.detection.objects) {
    QJsonObject jo;
    jo["label"] = obj.label;
    jo["confidence"] = obj.confidence;
    jo["bbox"] = QJsonArray{obj.bbox.x(), obj.bbox.y(), obj.bbox.width(), obj.bbox.height()};
    objects.push_back(jo);
  }
  telemetryQ.addBindValue(QString::fromUtf8(QJsonDocument(objects).toJson(QJsonDocument::Compact)));
  if (!telemetryQ.exec()) {
    qWarning() << "Insert telemetry for snapshot failed:" << telemetryQ.lastError().text();
    return;
  }

  const auto telemetryId = telemetryQ.lastInsertId().toLongLong();

  QSqlQuery gpsQ(db_);
  gpsQ.prepare("INSERT INTO gps_samples(telemetry_id,sample_ts_ms,time_usec,lat_e7,lon_e7,alt_mm,vel_cms,cog_cdeg,fix_type,satellites_visible,lat_deg,lon_deg) "
               "VALUES(?,?,?,?,?,?,?,?,?,?,?,?);");
  gpsQ.addBindValue(telemetryId);
  gpsQ.addBindValue(pkt.recvTsMs);
  gpsQ.addBindValue(pkt.gps.timeUsec);
  gpsQ.addBindValue(pkt.gps.latE7);
  gpsQ.addBindValue(pkt.gps.lonE7);
  gpsQ.addBindValue(pkt.gps.altMm);
  gpsQ.addBindValue(pkt.gps.velCms);
  gpsQ.addBindValue(pkt.gps.cogCdeg);
  gpsQ.addBindValue(pkt.gps.fixType);
  gpsQ.addBindValue(pkt.gps.satellitesVisible);
  gpsQ.addBindValue(static_cast<double>(pkt.gps.latE7) / 10000000.0);
  gpsQ.addBindValue(static_cast<double>(pkt.gps.lonE7) / 10000000.0);
  gpsQ.exec();

  QSqlQuery q(db_);
  q.prepare("INSERT INTO snapshot_events(event_ts_ms,telemetry_id,screenshot_path,reason_tag,label,confidence,is_target_event) "
            "VALUES(?,?,?,?,?,?,?);");
  q.addBindValue(QDateTime::currentMSecsSinceEpoch());
  q.addBindValue(telemetryId);
  q.addBindValue(screenshotPath);
  q.addBindValue(reasonTag);
  q.addBindValue(pkt.detection.label);
  q.addBindValue(pkt.detection.confidence);
  q.addBindValue(isTargetEvent ? 1 : 0);
  if (!q.exec()) {
    qWarning() << "Insert snapshot_events failed:" << q.lastError().text();
  }
}

void SQLiteDatabaseService::insertAppLogAsync(qint64 tsMs, const QString& level, const QString& type,
                                              const QString& message) {
  if (!ensureConnection()) return;
  QSqlQuery q(db_);
  q.prepare("INSERT INTO app_logs(ts_ms,level,type,message) VALUES(?,?,?,?);");
  q.addBindValue(tsMs);
  q.addBindValue(level);
  q.addBindValue(type);
  q.addBindValue(message);
  if (!q.exec()) {
    qWarning() << "Insert app_log failed:" << q.lastError().text();
  }
}

void SQLiteDatabaseService::insertPlaybackIndexAsync(const demo::client::PlaybackIndexRecord& rec) {
  if (!ensureConnection()) return;
  QSqlQuery q(db_);
  q.prepare("INSERT INTO playback_index(frame_ts_ms,wall_ts_ms,meta_path,latency_ms,label,confidence) VALUES(?,?,?,?,?,?);");
  q.addBindValue(rec.frameTsMs);
  q.addBindValue(rec.wallTsMs);
  q.addBindValue(rec.metaPath);
  q.addBindValue(rec.latencyMs);
  q.addBindValue(rec.label);
  q.addBindValue(rec.confidence);
  if (!q.exec()) {
    qWarning() << "Insert playback_index failed:" << q.lastError().text();
  }
}

QList<EventRecord> SQLiteDatabaseService::queryEvents(const QString& label, qint64 fromMs, qint64 toMs,
                                                      int limit) {
  QList<EventRecord> items;
  if (!ensureConnection()) return items;

  QString sql = "SELECT event_ts_ms, screenshot_path, label, confidence, is_target_event FROM snapshot_events "
                "WHERE is_target_event = 1 AND event_ts_ms BETWEEN ? AND ?";
  if (!label.trimmed().isEmpty()) {
    sql += " AND label = ?";
  }
  sql += " ORDER BY event_ts_ms DESC LIMIT ?";

  QSqlQuery q(db_);
  q.prepare(sql);
  q.addBindValue(fromMs);
  q.addBindValue(toMs);
  if (!label.trimmed().isEmpty()) {
    q.addBindValue(label.trimmed());
  }
  q.addBindValue(limit);
  if (!q.exec()) {
    qWarning() << "Query events failed:" << q.lastError().text();
    return items;
  }

  while (q.next()) {
    EventRecord r;
    r.tsMs = q.value(0).toLongLong();
    r.screenshotPath = q.value(1).toString();
    r.label = q.value(2).toString();
    r.confidence = q.value(3).toDouble();
    r.isTargetEvent = q.value(4).toInt() != 0;
    items.push_back(r);
  }
  return items;
}

} // namespace demo::client
