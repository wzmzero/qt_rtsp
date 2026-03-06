#pragma once

#include <QObject>
#include <QVideoFrame>

QT_BEGIN_NAMESPACE
class QMediaPlayer;
class QVideoSink;
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

private:
  QMediaPlayer* player_{nullptr};
  QVideoSink* sink_{nullptr};
};

} // namespace demo::client
