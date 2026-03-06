# qt_rtsp_tcp_project

Qt6 客户端 + C++ 模拟服务端示例，支持 RTSP 视频与 TCP 遥测融合展示。
本次补齐了 **SQLite3（QtSql）数据访问层 + Repository 抽象 + 主窗口接入**。

## 主要能力

- 多线程解耦：TCP / RTSP / Record / DB
- 配置可编辑并持久化（QSettings + SQLite）
- 遥测（检测结果 + GPS）持续入库
- UI 不直接写 SQL，统一走 Repository
- 数据库写入通过 `QueuedConnection` 异步投递，避免阻塞 UI

## 架构分层

```text
src/client/
  app/                  # MainWindow(UI编排)
  core/                 # DTO + AppConfig
  network/              # TcpClientWorker (含重连)
  media/                # StreamWorker / RecordWorker
  database/             # IDatabaseService + SQLiteDatabaseService
  repository/           # IAppRepository + AppRepository
```

## SQLite Schema（v1）

- `schema_version(version)`
- `app_config`
  - `id=1`
  - `rtsp_url, tcp_host, tcp_port, reconnect_ms`
  - `record_dir, record_enabled, theme`
  - `window_geometry, window_state, updated_at_ms`
- `telemetry_results`
  - `id, label, confidence, source_ts_ms, sent_ts_ms, recv_ts_ms`
  - `gps_time_usec, gps_lat_e7, gps_lon_e7, gps_alt_mm`
  - `gps_vel_cms, gps_cog_cdeg, gps_fix_type, gps_satellites_visible`

数据库默认位置：
- Linux: `~/.local/share/qt_client/client_data.sqlite`

## 构建

```bash
cmake -S . -B build
cmake --build build -j4
```

## 运行

### 1) 启动模拟服务端

```bash
./build/src/server/sim_server 9000 sample.mp4
```

### 2) 启动客户端

```bash
./build/src/client/qt_client
```

## 使用说明（数据库相关）

1. 首次启动客户端会自动初始化 DB，并创建 `schema_version/app_config/telemetry_results`。
2. 点击“保存配置”或“启动全部”会触发 `repo_->saveConfig()`：
   - 立即写 QSettings（前台）
   - 异步写 SQLite（DB 线程）
3. TCP 收到遥测后，UI 更新展示，并异步调用 `repo_->appendTelemetry(pkt)` 入库。
4. 启动时配置读取顺序：先读 SQLite，再由 QSettings 覆盖（兼容历史设置）。
