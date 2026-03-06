# qt_rtsp_tcp_project

Qt6 客户端 + C++ 模拟服务端示例，支持 RTSP 视频与 TCP 遥测融合展示。

## 产品化阶段（当前状态）

### 客户端功能
- 多页面架构（`QStackedWidget`）：
  - 实时模式（拉流 + 状态 + 参数 + 截图）
  - 回放模式（当前为占位页，已接入 `playback_index` 入库）
  - 日志查看（级别/类型过滤）
  - 事件查看（标签 + 时间范围查询）
- 菜单负责切页与轻量动作（截图、刷新事件）；参数区保留启动/停止/保存配置。
- 保存配置后弹窗提示成功，主题在保存后立即生效。
- 主窗口保持原生拖动、最大化、缩放能力（非固定尺寸）。
- GPS 同时展示原始值（E7）与解析值（度）。

## SQLite 数据库

数据库路径：
- 状态栏显示：`数据库: .../client_data.sqlite`
- Linux 常见路径（本项目默认）：
  - `~/.local/share/qt_client/client_data.sqlite`

Schema 版本：`3`（含迁移）

### 表结构（按功能分表）
1. `app_config`
   - 单行配置（`id=1`）：连接参数、录制参数、主题、窗口几何信息。
2. `telemetry_results`
   - 遥测检测主表：`recv/sent/source` 时间戳、`label`、`confidence`。
3. `gps_samples`
   - GPS 明细：原始字段（E7、速度等）+ 解析字段（`lat_deg/lon_deg`），通过 `telemetry_id` 关联遥测。
4. `snapshot_events`
   - 截图事件：截图路径、事件时间、原因标签（manual/legacy）、目标事件标记等。
5. `app_logs`
   - 应用日志持久化：`ts_ms/level/type/message`。
6. `playback_index`
   - 回放索引：帧时间、落盘时间、meta 文件路径、延迟、标签/置信度。
7. `schema_version`
   - 版本控制与迁移记录：`version/applied_at_ms`。

### 迁移逻辑
- 旧库（`schema_version` 单列）会自动规范化为双列版本表。
- 旧表 `telemetry_objects` 自动迁移到：
  - `telemetry_results`
  - `gps_samples`
  - `snapshot_events`
- 迁移完成后写入 `schema_version = 3`，并清理旧 `telemetry_objects`。

## 构建

```bash
cmake -S . -B build
cmake --build build -j4
```

## 运行

### 1) 启动 RTSP 服务端（如 mediamtx）
```bash
mediamtx
```

### 2) 启动 sim_server（自动推流 + TCP 模拟）
```bash
./build/src/server/sim_server
```

### 3) 启动客户端
```bash
./build/src/client/qt_client
```

## 验证记录（本次）
- 编译验证：`cmake -S . -B build && cmake --build build -j4` ✅
- 运行冒烟：`QT_QPA_PLATFORM=offscreen ./build/src/client/qt_client`（`timeout 5s` 控制退出）✅


## 路径策略（发布版）

当前采用“**相对当前工程/运行目录优先**”策略，默认写入：
- `./data/client_data.sqlite`（SQLite）
- `./logs/qt_client.log`（文本日志）
- `./snapshots/`（截图）
- `./recordings/record_meta.jsonl`（录制元数据）
- `playback_index.meta_path` 也优先保存相对路径

配置覆盖仍可用（支持绝对路径），但默认模板与保存值优先使用相对路径，避免绑定宿主机目录。

## person + rod 组合告警（基础框架）

- 支持从检测对象列表读取 `label/confidence/bbox`。
- 对 person 与 rod 框计算 IoU，默认阈值 `0.10`。
- 命中后更新 UI 告警状态灯/状态文字，并写入日志（DB + `./logs/qt_client.log`）。
- 检测对象列表同时以 JSON 字符串写入 SQLite（`telemetry_results.detection_objects_json`）。

## Release 构建与交付

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release -j4
```

产物目录：`release/`（含 `bin/ conf/ scripts/ media/ README_RELEASE.md`）。

## sim_server 可靠性增强（2026-03-06）

已完成：TCP 与 RTSP 子系统彻底解耦，分别由独立 supervisor 线程管理；任一子系统失败不会导致主进程退出。

### 新增运行参数

```bash
./build-release/src/server/sim_server \
  --tcp-port 9200 \
  --video media/test.mp4 \
  --rtsp-url rtsp://127.0.0.1:8554/live \
  --tcp-retry-ms 2000 \
  --rtsp-retry-min-ms 1000 \
  --rtsp-retry-max-ms 10000 \
  --health-interval-sec 5
```

参数说明：
- `--tcp-retry-ms`：TCP bind/listen 失败后的固定重试间隔
- `--rtsp-retry-min-ms / --rtsp-retry-max-ms`：RTSP 推流失败后的指数退避区间
- `--health-interval-sec`：健康状态打印周期

### 日志与健康状态

启动后会周期打印：
- TCP 状态（RUNNING/RETRY_WAIT/EXCEPTION/STOPPED）
- RTSP 状态（RUNNING/RETRY_WAIT/EXCEPTION/STOPPED）
- 各子系统重试次数
- 最近一次错误信息

示例：

```
[sim_server][health] tcp={status:RUNNING,retries:2,last_error:'run() returned non-zero'} rtsp={status:RUNNING,retries:0,last_error:''}
```

