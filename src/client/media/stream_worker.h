#pragma once

#include <QObject>
#include <QVideoFrame>
#include <QUrl>

QT_BEGIN_NAMESPACE
class QMediaPlayer;
class QVideoSink;
class QTimer;
QT_END_NAMESPACE

namespace demo::client {

class StreamWorker : public QObject {
  Q_OBJECT
public:
  explicit StreamWorker(QObject* parent = nullptr);

public slots:
  void start(const QUrl& rtspUrl);
  void stop();

signals:
  void frameArrived(const QVideoFrame& frame, qint64 captureTsMs);
  void logMessage(const QString& msg);

private slots:
  void onFrameChanged(const QVideoFrame& frame);
  void onPlayerError();
  void checkStreamAlive();

private:
  QMediaPlayer* player_{nullptr};
  QVideoSink* sink_{nullptr};
  QTimer* watchdog_{nullptr};
  QUrl baseUrl_;
  qint64 lastFrameTsMs_{0};
  bool triedTcpParam_{false};
};

} // namespace demo::client
