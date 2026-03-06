#include "media/stream_worker.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QTimer>
#include <iostream>

int main(int argc, char** argv) {
  QCoreApplication app(argc, argv);

  QString url = "rtsp://127.0.0.1:8554/live";
  int timeoutMs = 8000;
  for (int i = 1; i < argc; ++i) {
    const QString a = argv[i];
    if ((a == "-u" || a == "--url") && i + 1 < argc) url = argv[++i];
    if ((a == "-t" || a == "--timeout-ms") && i + 1 < argc) timeoutMs = QString(argv[++i]).toInt();
  }

  demo::client::StreamWorker worker;
  int frames = 0;
  QStringList logs;
  QElapsedTimer timer;
  timer.start();

  QObject::connect(&worker, &demo::client::StreamWorker::frameArrived, &app,
                   [&](const QVideoFrame&, qint64) { ++frames; });
  QObject::connect(&worker, &demo::client::StreamWorker::logMessage, &app,
                   [&](const QString& m) { logs << m; });

  QTimer::singleShot(0, &app, [&]() { worker.start(QUrl(url)); });
  QTimer::singleShot(timeoutMs, &app, [&]() {
    worker.stop();
    std::cout << "frames=" << frames << " elapsed_ms=" << timer.elapsed() << "\n";
    for (const auto& l : logs) std::cout << l.toStdString() << "\n";
    app.exit(frames > 0 ? 0 : 2);
  });

  return app.exec();
}
