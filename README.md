# qt_rtsp_tcp_project

Qt6 客户端 + C++ 模拟服务端示例，支持 RTSP 视频与 TCP 遥测融合展示，并完成了 **UI 升级 + 可配置化 + SQLite 持久化**。

## 主要能力

- 三线程解耦（TCP / RTSP / Record）+ UI 编排线程
- 新增数据库线程（SQLite + QtSql）用于配置与结果持久化
- 可配置参数（界面可改 + 持久化）：
  - RTSP URL
  - TCP Host / Port
  - TCP 重连间隔
  - 录制目录 / 录制开关
  - 主题、窗口布局
- UI 重构：
  - 顶部菜单栏（文件/连接/视图/帮助）
  - 工具栏（启动/停止/保存配置）
  - 主视频区域居中且占主区域
  - 配置 GroupBox + 底部日志 + 状态面板（连接状态/时间戳/检测/GPS）

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

- **UI 不直接操作 SQL**，通过 Repository -> DatabaseService 抽象访问。
- **数据库写入异步化**：配置保存与 telemetry 入库通过 `QueuedConnection` 投递到 DB 线程，避免阻塞 UI。

## SQLite Schema（v1）

- `schema_version(version)`
- `app_config`
  - `rtsp_url, tcp_host, tcp_port, reconnect_ms, record_dir, record_enabled, theme, window_geometry, window_state, updated_at_ms`
- `telemetry_results`
  - `label, confidence, source_ts_ms, sent_ts_ms, recv_ts_ms`
  - `gps_time_usec, gps_lat_e7, gps_lon_e7, gps_alt_mm, gps_vel_cms, gps_cog_cdeg, gps_fix_type, gps_satellites_visible`

数据库文件默认位置：
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

在 UI 中修改参数并点击“保存配置”，会同时写入：
- QSettings（快速本地配置）
- SQLite（结构化配置 + 可扩展查询）

## 已知限制

1. 当前仅实现最小迁移入口（`schema_version` + v1 建表），未实现多版本自动回滚策略。
2. `StreamWorker` 未实现 RTSP 自动重连状态机（TCP 已支持重连）。
3. RecordWorker 当前落盘为抽样 JSONL 元数据，未做完整视频编码归档。
