#pragma once

#include "core/app_config.h"
#include "core/types.h"

#include <QObject>
#include <QByteArray>
#include <QtSql/QSqlDatabase>

namespace demo::client {

struct EventRecord {
  qint64 tsMs{0};
  QString screenshotPath;
  QByteArray screenshotBlob;
  QString reasonTag;
  bool isTargetEvent{false};
  double latDeg{0.0};
  double lonDeg{0.0};
};

class IDatabaseService {
public:
  virtual ~IDatabaseService() = default;
  virtual bool initialize() = 0;
  virtual AppConfig loadConfig() = 0;
};

struct PlaybackIndexRecord {
  qint64 frameTsMs{0};
  qint64 wallTsMs{0};
  QString metaPath;
  qint64 latencyMs{0};
  QString label;
  double confidence{0.0};
  double latDeg{0.0};
  double lonDeg{0.0};
};

class SQLiteDatabaseService : public QObject, public IDatabaseService {
  Q_OBJECT
public:
  explicit SQLiteDatabaseService(const QString& dbPath, QObject* parent = nullptr);
  ~SQLiteDatabaseService() override;

  bool initialize() override;
  AppConfig loadConfig() override;
  QString dbPath() const { return dbPath_; }

  QList<EventRecord> queryEvents(const QString& label, qint64 fromMs, qint64 toMs, int limit);
  QList<PlaybackIndexRecord> queryPlaybackIndex(int limit);

public slots:
  void saveConfigAsync(const demo::client::AppConfig& cfg);
  void insertTelemetryAsync(const demo::client::TelemetryPacket& pkt);
  void insertSnapshotEventAsync(const demo::client::TelemetryPacket& pkt, const QByteArray& screenshotBlob,
                                const QString& reasonTag, bool isTargetEvent);
  void insertAppLogAsync(qint64 tsMs, const QString& level, const QString& type, const QString& message);
  void insertPlaybackIndexAsync(const demo::client::PlaybackIndexRecord& rec);

private:
  bool ensureConnection();
  bool migrate();
  QString connectionNameForCurrentThread() const;

  QString dbPath_;
  QSqlDatabase db_;
};

} // namespace demo::client

Q_DECLARE_METATYPE(demo::client::EventRecord)
Q_DECLARE_METATYPE(QList<demo::client::EventRecord>)

Q_DECLARE_METATYPE(demo::client::PlaybackIndexRecord)
