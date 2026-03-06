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
