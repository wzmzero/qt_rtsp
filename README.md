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
