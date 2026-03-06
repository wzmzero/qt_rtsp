# PROGRESS

## 2026-03-06（数据库接口层收口：QtSql + Repository + 主窗口接入）

### 本次目标
1. 在客户端补齐 SQLite3 数据访问层（QtSql）。
2. 引入 DatabaseService / Repository 抽象，避免 UI 直接 SQL。
3. 实现 schema_version + 初始化建表。
4. 配置读写与检测结果+GPS 入库。
5. 通过异步写入避免阻塞 UI。
6. 主窗口接入仓储接口。
7. 更新文档并完成 configure/build 验证。

### 关键实现
- 新增/完善 `src/client/database/database_service.*`
  - `IDatabaseService` 抽象
  - `SQLiteDatabaseService` 实现
  - `initialize()` / `migrate()`
  - `schema_version` 版本控制（v1）
  - `app_config`、`telemetry_results` 建表
  - `loadConfig()` + `saveConfigAsync()` + `insertTelemetryAsync()`
  - 按线程创建连接名（`QThread::currentThreadId`）避免跨线程复用连接

- 新增/完善 `src/client/repository/app_repository.*`
  - `IAppRepository` 抽象
  - 配置读取：SQLite（BlockingQueuedConnection）+ QSettings 覆盖
  - 配置保存：QSettings 同步 + SQLite 异步投递
  - telemetry 入库：QueuedConnection 异步投递

- 主窗口接入仓储（`src/client/app/main_window.*`）
  - 增加 `dbThread_`、`SQLiteDatabaseService`、`AppRepository`
  - 启动时在 DB 线程执行 `initialize()`
  - 启动/保存/析构时统一走 `repo_->saveConfig()`
  - 收到 telemetry 时 `repo_->appendTelemetry(pkt)`

- 构建系统更新（`src/client/CMakeLists.txt`）
  - 启用 `Qt6::Sql`
  - 编译 `database/database_service.cpp` 与 `repository/app_repository.cpp`

### 数据库 Schema（v1）
- `schema_version(version)`
- `app_config`
  - `id=1, rtsp_url, tcp_host, tcp_port, reconnect_ms, record_dir, record_enabled, theme, window_geometry, window_state, updated_at_ms`
- `telemetry_results`
  - `id, label, confidence, source_ts_ms, sent_ts_ms, recv_ts_ms, gps_time_usec, gps_lat_e7, gps_lon_e7, gps_alt_mm, gps_vel_cms, gps_cog_cdeg, gps_fix_type, gps_satellites_visible`

### 验证
执行：
```bash
cmake -S . -B build
cmake --build build -j4
```
结果：
- Configure：成功（server 目标存在 AUTOGEN dev warning，不影响）
- Build：成功
- 产物：`build/src/client/qt_client`、`build/src/server/sim_server`
