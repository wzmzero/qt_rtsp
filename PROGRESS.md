# PROGRESS

## 2026-03-06（产品化改造收尾）

### 本轮目标
- 数据库从“两大表”修正为“按功能分表”。
- 提供 `schema_version` 迁移逻辑，兼容已有库升级。
- 保持多页面 UI 与参数区职责边界。
- 完成截图事件入库、日志筛选基础能力、GPS 原始+解析显示。
- 更新文档并完成可编译可运行验证。

### 已完成
1. **数据库重构（v3）**
   - 新/保留表：
     - `app_config`
     - `telemetry_results`
     - `gps_samples`
     - `snapshot_events`
     - `app_logs`
     - `playback_index`
     - `schema_version`
   - `schema_version` 标准化为 `(version, applied_at_ms)`。

2. **迁移逻辑**
   - 兼容旧 `schema_version(version)` 单列结构，自动标准化。
   - 兼容旧 `telemetry_objects`，迁移到：
     - `telemetry_results`（检测信息）
     - `gps_samples`（GPS 原始+解析）
     - `snapshot_events`（截图/目标事件）
   - 迁移后写入 `schema_version=3` 并删除旧 `telemetry_objects`。

3. **UI 架构与交互（保持并补齐）**
   - 维持 `实时/回放/日志/事件` 四页结构。
   - 菜单负责切页；参数区保留启动/停止/保存配置。
   - 保存配置成功弹窗已生效；主题保存后即时生效。
   - 主窗口保留拖动、缩放行为（未固定尺寸，设置最小尺寸保障可用性）。

4. **功能闭环**
   - 截图事件写入 `snapshot_events`（含 reasonTag、目标事件标记）。
   - 日志筛选保留（级别/类型），并新增落库到 `app_logs`。
   - GPS 同时展示原始 E7 + 解析度数；入库至 `gps_samples`。
   - 录制抽样时写入 `playback_index`。

5. **构建/运行验证**
   - `cmake -S . -B build && cmake --build build -j4` 通过。
   - `QT_QPA_PLATFORM=offscreen` 启动客户端 5 秒冒烟验证通过。

### 变更文件
- `src/client/database/database_service.h`
- `src/client/database/database_service.cpp`
- `src/client/repository/app_repository.cpp`
- `src/client/media/record_worker.h`
- `src/client/media/record_worker.cpp`
- `src/client/app/main_window.cpp`
- `src/client/app/main.cpp`
- `README.md`
- `PROGRESS.md`

### 已知限制
1. 回放页仍为功能占位，尚未实现播放器列表/时间轴 UI。
2. `insertSnapshotEventAsync` 当前会额外插入一条 telemetry+gps（用于保证事件关联），后续可引入去重策略。
3. 事件目标规则目前仍是 `label == person`（可配置化待后续迭代）。


## 2026-03-06（Release 收尾：相对路径 + 组合告警）

### 新增与调整
1. **持久化路径统一相对策略**（运行目录优先）
   - SQLite: `./data/client_data.sqlite`
   - 文本日志: `./logs/qt_client.log`
   - 截图: `./snapshots/`
   - 录制元数据: `./recordings/record_meta.jsonl`
   - 回放索引 `meta_path` 优先记录相对路径

2. **配置覆盖保留**
   - 支持绝对路径覆盖；默认配置模板与保存值优先相对路径（`./data ./logs ./snapshots ./recordings`）。
   - 客户端配置文件迁移到相对目录：`./conf/client.ini`。

3. **person+rod 组合告警最小可用实现**
   - TCP 检测消息支持对象列表：`label/confidence/bbox`。
   - IoU 判断（阈值默认 0.10），触发后更新 UI 告警灯/状态文本，并写日志。
   - 检测对象 JSON 入库：`telemetry_results.detection_objects_json`。

4. **Release 构建验证**
   - `cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release` ✅
   - `cmake --build build-release -j4` ✅

5. **发布目录产出**
   - `release/bin/qt_client`
   - `release/bin/sim_server`
   - `release/conf/client.ini(.template)`
   - `release/scripts/run_server.sh`, `run_client.sh`
   - `release/media/test.mp4` + README
   - `release/README_RELEASE.md`


## 2026-03-06（sim_server 可靠性修复：TCP/RTSP 解耦 + 自愈）

### 根因分析
1. **生命周期耦合**：旧实现主线程先 `launcher.start()` 后直接 `TcpSimServer::run()`，TCP/RTSP 缺少独立监督循环，异常恢复能力弱。
2. **失败即返回**：TCP 在 `bind/listen` 失败时直接返回错误码，导致上层无重试；RTSP 仅做启动期检查，运行中 ffmpeg 退出无法自动恢复。
3. **可观测性不足**：没有统一健康状态输出，不便定位“哪一侧挂了、重试了几次、最后错误是什么”。
4. **隐藏 FD 继承风险**：RTSP 进程 fork/exec 时会继承 TCP socket（未设 CLOEXEC），可造成端口占用混淆。

### 修改点
1. **主进程改为双 supervisor 线程**（彻底解耦）
   - `run_tcp_loop()`：独立管理 TCP 子系统
   - `run_rtsp_loop()`：独立管理 RTSP 子系统
   - 两者互不依赖生命周期；任一失败不会拉垮主进程。

2. **TCP 自愈重试**
   - `TcpSimServer::run()` 保持单次运行语义；`main` 中 supervisor 对非 0 返回进行重试。
   - 新增 `--tcp-retry-ms` 可配置重试间隔。

3. **RTSP 自愈重试 + 退避**
   - `RtspLauncher` 新增 `check_alive()`，检测 ffmpeg 是否中途退出。
   - supervisor 在启动失败/中途退出时自动重启，采用指数退避（`--rtsp-retry-min-ms/--rtsp-retry-max-ms`）。

4. **异常防护**
   - TCP/RTSP supervisor 循环与健康输出均加 `try/catch`，异常记录后继续服务。

5. **日志与健康状态**
   - 统一前缀 `[sim_server][tcp|rtsp|health]`
   - 打印状态、重试次数、最后错误
   - 新增 `--health-interval-sec` 控制周期输出

6. **FD 继承修复**
   - TCP `listen_fd/client_fd` 设置 `FD_CLOEXEC`，避免 ffmpeg 继承 socket。

### 实机测试与证据

1) **mediamtx 复用**
- 进程已在运行：`ps -fp 36445` -> `./mediamtx`

2) **启动 sim_server（稳定用例）**
```bash
stdbuf -oL -eL ./build-release/src/server/sim_server \
  --tcp-port 9200 \
  --video media/test.mp4 \
  --rtsp-url rtsp://127.0.0.1:8554/live_robust \
  --health-interval-sec 5 \
  > /tmp/sim_server_robust.log 2>&1
```

3) **验证 TCP 监听**
```bash
ss -lntp | grep ':9200'
lsof -iTCP:9200 -sTCP:LISTEN -nP
```
关键输出：
- `LISTEN ... 0.0.0.0:9200 ... ("sim_server",pid=37313,fd=3)`

4) **验证 RTSP 发布会话持续 >=20s**
```bash
# T0
lsof -iTCP:8554 -nP | grep ESTABLISHED
# sleep 22
# T1
lsof -iTCP:8554 -nP | grep ESTABLISHED
```
关键输出（T0=16:09:51, T1=16:10:13）：
- T0: `ffmpeg 37316 ... 127.0.0.1:45692->127.0.0.1:8554 (ESTABLISHED)`
- T1: `ffmpeg 37316 ... 127.0.0.1:45692->127.0.0.1:8554 (ESTABLISHED)`

5) **TCP bind 失败不退出（重试生效）**
- 以同端口启动第二实例触发冲突，日志持续出现：
  - `[sim_server][tcp] bind failed: Address already in use`
  - `[sim_server][tcp] restarting after failure, retry=...`
  - 同时 RTSP 仍 `RUNNING`

6) **RTSP 启动失败不退出（退避重试生效）**
- 使用不存在的 ffmpeg：`--ffmpeg /no/such/ffmpeg`
- 日志持续出现：
  - `ERROR: exec ffmpeg failed`
  - `startup failed, retry=1 backoff_ms=500`
  - `retry=2 backoff_ms=1000`
  - `retry=3 backoff_ms=2000`（封顶后维持）
- 同时 TCP 仍 `listening on 0.0.0.0:9300`

### 变更文件
- `src/server/app/main.cpp`
- `src/server/network/tcp_sim_server.h`
- `src/server/network/tcp_sim_server.cpp`
- `src/server/media/rtsp_launcher.h`
- `src/server/media/rtsp_launcher.cpp`
- `README.md`
- `PROGRESS.md`
