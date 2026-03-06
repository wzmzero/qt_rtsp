# qt_rtsp_tcp_project

Qt6 客户端 + C++ 模拟服务端示例，支持 RTSP 视频与 TCP 遥测融合展示。

## 产品化阶段（当前状态）

## 2026-03-06 紧急UI重构与告警规则修正（最新）
- 实时页改为“左视频主区 + 右侧纵向三块（连接状态/数据接收/参数编辑）”。
- 连接状态仅保留三个状态灯：RTSP、TCP、响应绑定。
- 原始数据仅展示最近N条摘要，提供“查看完整”弹窗。
- 告警规则改为：同消息 `person + rod` 且各自置信度过阈值，不再计算 IoU。
- 告警三档：低(浅黄)/中(深黄)/高(红)，阈值默认 `0.5/0.7/0.85` 且可配置持久化。
- 主题菜单与当前主题双向绑定；保存配置后保持当前主题。

### 客户端功能
- 多页面架构（`QStackedWidget`）：
  - 实时（主区域整合：视频、连接/告警、接收数据、解析结果、参数编辑）
  - 回放（当前为占位页，已接入 `playback_index` 入库）
  - 日志（级别/类型过滤）
  - 事件（标签 + 时间范围查询）
- 菜单栏固定四项（全局动作统一收口）：
  - 文件(File)：保存配置、退出
  - 页面(Page)：实时/回放/日志/事件切换
  - 功能(Function)：截图、刷新事件、主题（暗/明）
  - 帮助(Help)：关于
- 参数区仅保留运行参数与 启动/停止；保存配置/主题切换均在菜单栏执行。
- 主界面按“左主右辅”优化比例（左：视频+状态，右：参数+接收/解析），可读性更高。
- GPS 同时展示原始值（E7）与解析值（度）。

## SQLite 数据库

数据库路径：
- 状态栏显示（相对路径）：`运行中 | DB: ./config/client_cfg.db`
- 默认落盘路径：`./config/client_cfg.db`
- 兼容旧库：若存在 `./data/client_data.sqlite` 且新库不存在，启动时自动一次性迁移复制到新路径。

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
- `./config/client_cfg.db`（SQLite）
- `./logs/qt_client.log`（文本日志）
- `./snapshots/`（截图）
- `./recordings/record_meta.jsonl`（录制元数据）
- `playback_index.meta_path` 也优先保存相对路径

配置覆盖仍可用（支持绝对路径），但默认模板与保存值优先使用相对路径，避免绑定宿主机目录。

## person + rod 告警（当前规则）

- 输入字段：`label/confidence/bbox + source_ts_ms/sent_ts_ms + GPS`。
- 规则：同一消息内同时存在 person 与 rod，且 `person_conf/rod_conf` 同时达到阈值即触发。
- 告警分级：低(浅黄)/中(深黄)/高(红)。
- 阈值默认：`low=0.50, mid=0.70, high=0.85`，可在参数区配置并持久化。

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



## 2026-03-06 重点改造验证（UI + 协议 + 告警）

### 协议
- `sim_server` 推送多目标检测，字段包含 `label/confidence/cx/cy/w/h`（归一化 YOLO 坐标）并保留 `bbox` 兼容字段。
- 每条遥测包含 `source_ts_ms/sent_ts_ms` 与 GPS 原始字段。

### 客户端解析与算法
- 解析多目标对象列表（支持 `cx/cy/w/h` 或 `bbox`）。
- 告警规则：同消息 person+rod 且双方置信度达到阈值，按 low/mid/high 三档告警。
- GPS 同时展示原始 E7 与解析经纬度（度）。

### 实测命令
```bash
cmake -S . -B build
cmake --build build -j4

./build/src/server/sim_server
QT_QPA_PLATFORM=offscreen QT_CLIENT_AUTOTEST=1 ./build/src/client/qt_client
```

### 关键证据（摘自 `./logs/qt_client.log`）
- `StatusBar => 运行中 | DB: ./config/client_cfg.db`
- `告警级别切换: low/mid/high (person=..., rod=...)`
- `StatusBar => 已停止 | DB: ./config/client_cfg.db`

### 已知限制
- 当前 CI/无头环境通过 `QT_CLIENT_AUTOTEST` 自动触发“启动/保存/停止”流程验证按钮行为；完整可视化点击验证建议在桌面环境补测。
