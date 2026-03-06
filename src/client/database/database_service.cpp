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

namespace {
int labelIdFromName(const QString& name) {
  const auto n = name.trimmed().toLower();
  if (n == "person") return 0;
  if (n == "rod") return 1;
  return -1;
}

qint64 persistTelemetryPacket(QSqlDatabase& db, const demo::client::TelemetryPacket& pkt) {
  QSqlQuery t(db);
  t.prepare("INSERT INTO telemetry(recv_ts_ms,sent_ts_ms,source_ts_ms) VALUES(?,?,?);");
  t.addBindValue(pkt.recvTsMs);
  t.addBindValue(pkt.sentTsMs);
  t.addBindValue(pkt.detection.sourceTsMs);
  if (!t.exec()) {
    qWarning() << "Insert telemetry failed:" << t.lastError().text();
    return -1;
  }
  const qint64 telemetryId = t.lastInsertId().toLongLong();

  QSqlQuery d(db);
  d.prepare("INSERT INTO detection_object(telemetry_id,obj_index,label_id,label,confidence,cx,cy,w,h) VALUES(?,?,?,?,?,?,?,?,?);");
  for (int i = 0; i < pkt.detection.objects.size(); ++i) {
    const auto& obj = pkt.detection.objects[i];
    d.addBindValue(telemetryId);
    d.addBindValue(i);
    d.addBindValue(labelIdFromName(obj.label));
    d.addBindValue(obj.label);
    d.addBindValue(obj.confidence);
    d.addBindValue(obj.bbox.x());
    d.addBindValue(obj.bbox.y());
    d.addBindValue(obj.bbox.width());
    d.addBindValue(obj.bbox.height());
    if (!d.exec()) {
      qWarning() << "Insert detection_object failed:" << d.lastError().text();
    }
    d.finish();
  }

  QSqlQuery g(db);
  g.prepare("INSERT INTO gps(telemetry_id,time_usec,lat_e7,lon_e7,alt_mm,eph_cm,epv_cm,vel_cms,cog_cdeg,fix_type,satellites_visible) "
            "VALUES(?,?,?,?,?,?,?,?,?,?,?);");
  g.addBindValue(telemetryId);
  g.addBindValue(pkt.gps.timeUsec);
  g.addBindValue(pkt.gps.latE7);
  g.addBindValue(pkt.gps.lonE7);
  g.addBindValue(pkt.gps.altMm);
  g.addBindValue(0);
  g.addBindValue(0);
  g.addBindValue(pkt.gps.velCms);
  g.addBindValue(pkt.gps.cogCdeg);
  g.addBindValue(pkt.gps.fixType);
  g.addBindValue(pkt.gps.satellitesVisible);
  if (!g.exec()) {
    qWarning() << "Insert gps failed:" << g.lastError().text();
  }

  return telemetryId;
}
} // namespace


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

  // --- core tables ---
  if (!q.exec("CREATE TABLE IF NOT EXISTS app_config("
              "id INTEGER PRIMARY KEY CHECK(id=1),"
              "rtsp_url TEXT NOT NULL, tcp_host TEXT NOT NULL, tcp_port INTEGER NOT NULL,"
              "reconnect_ms INTEGER NOT NULL, record_dir TEXT NOT NULL, record_enabled INTEGER NOT NULL,"
              "theme TEXT NOT NULL, window_geometry BLOB, window_state BLOB, updated_at_ms INTEGER NOT NULL);")) {
    qWarning() << "Create app_config failed:" << q.lastError().text();
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

  // --- proto-aligned telemetry storage ---
  if (!q.exec("CREATE TABLE IF NOT EXISTS telemetry("
              "id INTEGER PRIMARY KEY AUTOINCREMENT,"
              "recv_ts_ms INTEGER NOT NULL,"
              "sent_ts_ms INTEGER NOT NULL,"
              "source_ts_ms INTEGER NOT NULL);")) {
    qWarning() << "Create telemetry failed:" << q.lastError().text();
    return false;
  }

  if (!q.exec("CREATE TABLE IF NOT EXISTS detection_object("
              "id INTEGER PRIMARY KEY AUTOINCREMENT,"
              "telemetry_id INTEGER NOT NULL,"
              "obj_index INTEGER NOT NULL,"
              "label_id INTEGER, label TEXT, confidence REAL, cx REAL, cy REAL, w REAL, h REAL,"
              "FOREIGN KEY(telemetry_id) REFERENCES telemetry(id));")) {
    qWarning() << "Create detection_object failed:" << q.lastError().text();
    return false;
  }

  if (!q.exec("CREATE TABLE IF NOT EXISTS gps("
              "id INTEGER PRIMARY KEY AUTOINCREMENT,"
              "telemetry_id INTEGER NOT NULL UNIQUE,"
              "time_usec INTEGER, lat_e7 INTEGER, lon_e7 INTEGER, alt_mm INTEGER,"
              "eph_cm INTEGER, epv_cm INTEGER, vel_cms INTEGER, cog_cdeg INTEGER,"
              "fix_type INTEGER, satellites_visible INTEGER,"
              "FOREIGN KEY(telemetry_id) REFERENCES telemetry(id));")) {
    qWarning() << "Create gps failed:" << q.lastError().text();
    return false;
  }

  if (!q.exec("CREATE TABLE IF NOT EXISTS snapshot_events("
              "id INTEGER PRIMARY KEY AUTOINCREMENT,"
              "event_ts_ms INTEGER NOT NULL,"
              "telemetry_id INTEGER,"
              "screenshot_blob BLOB,"
              "reason_tag TEXT NOT NULL,"
              "label TEXT, confidence REAL, is_target_event INTEGER NOT NULL DEFAULT 0,"
              "FOREIGN KEY(telemetry_id) REFERENCES telemetry(id));")) {
    qWarning() << "Create snapshot_events failed:" << q.lastError().text();
    return false;
  }

  // cleanup obsolete tables
  q.exec("DROP TABLE IF EXISTS telemetry_results;");
  q.exec("DROP TABLE IF EXISTS gps_samples;");
  q.exec("DROP TABLE IF EXISTS playback_index;");
  q.exec("DROP TABLE IF EXISTS schema_version;");

  // cleanup obsolete columns
  q.exec("ALTER TABLE snapshot_events DROP COLUMN screenshot_path;"); // ignored on old sqlite if unsupported

  q.exec("CREATE INDEX IF NOT EXISTS idx_logs_filter ON app_logs(level, type, ts_ms);");
  q.exec("CREATE INDEX IF NOT EXISTS idx_telemetry_recv ON telemetry(recv_ts_ms);");
  q.exec("CREATE INDEX IF NOT EXISTS idx_detection_tm ON detection_object(telemetry_id, obj_index);");
  q.exec("CREATE INDEX IF NOT EXISTS idx_gps_tm ON gps(telemetry_id);");
  q.exec("CREATE INDEX IF NOT EXISTS idx_snapshot_filter ON snapshot_events(is_target_event, event_ts_ms, label);");

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
  (void)persistTelemetryPacket(db_, pkt);
}


void SQLiteDatabaseService::insertSnapshotEventAsync(const demo::client::TelemetryPacket& pkt,
                                                     const QByteArray& screenshotBlob,
                                                     const QString& reasonTag,
                                                     bool isTargetEvent) {
  if (!ensureConnection()) return;
  if (screenshotBlob.isEmpty()) return;

  const auto telemetryId = persistTelemetryPacket(db_, pkt);
  if (telemetryId <= 0) return;

  QSqlQuery q(db_);
  q.prepare("INSERT INTO snapshot_events(event_ts_ms,telemetry_id,screenshot_blob,reason_tag,label,confidence,is_target_event) "
            "VALUES(?,?,?,?,?,?,?);");
  q.addBindValue(pkt.recvTsMs > 0 ? pkt.recvTsMs : QDateTime::currentMSecsSinceEpoch());
  q.addBindValue(telemetryId);
  q.addBindValue(screenshotBlob);
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
  Q_UNUSED(rec);
}

QList<EventRecord> SQLiteDatabaseService::queryEvents(const QString& label, qint64 fromMs, qint64 toMs,
                                                      int limit) {
  QList<EventRecord> items;
  if (!ensureConnection()) return items;

  QString sql = "SELECT se.event_ts_ms, se.screenshot_blob, se.label, se.confidence, se.is_target_event, "
                "IFNULL(CAST(gm.lat_e7 AS REAL)/10000000.0,0), IFNULL(CAST(gm.lon_e7 AS REAL)/10000000.0,0), "
                "IFNULL((SELECT GROUP_CONCAT(printf('%s:[%.2f,%.2f,%.2f,%.2f]', do.label, do.cx, do.cy, do.w, do.h), ' | ') "
                "        FROM detection_object do WHERE do.telemetry_id = se.telemetry_id ORDER BY do.obj_index LIMIT 2), '') "
                "FROM snapshot_events se "
                "LEFT JOIN gps gm ON gm.telemetry_id = se.telemetry_id "
                "WHERE se.is_target_event = 1 AND se.event_ts_ms BETWEEN ? AND ?";
  if (!label.trimmed().isEmpty()) {
    sql += " AND se.label = ?";
  }
  sql += " ORDER BY se.event_ts_ms DESC LIMIT ?";

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
    r.screenshotPath.clear();
    r.screenshotBlob = q.value(1).toByteArray();
    r.label = q.value(2).toString();
    r.confidence = q.value(3).toDouble();
    r.isTargetEvent = q.value(4).toInt() != 0;
    r.latDeg = q.value(5).toDouble();
    r.lonDeg = q.value(6).toDouble();
    r.bboxSummary = q.value(7).toString();
    items.push_back(r);
  }
  return items;
}


QList<PlaybackIndexRecord> SQLiteDatabaseService::queryPlaybackIndex(int limit) {
  Q_UNUSED(limit);
  return {};
}


} // namespace demo::client
