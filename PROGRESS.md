# PROGRESS

## 2026-03-06

### 本轮目标
- 补齐 Qt 客户端关键源码：
  - `src/client/app/main.cpp`
  - `src/client/app/main_window.cpp`
  - `src/client/network/tcp_client_worker.*`（沿用并对接）
  - `src/client/media/stream_worker.*`（沿用并对接）
  - `src/client/media/record_worker.*`（增强记录）
- 完成文档：`README.md`、`PROGRESS.md`
- 保持模块化分层与多线程解耦
- 执行 configure/build 验证

### 已完成
1. **Qt 客户端入口与窗口实现**
   - 新增 `app/main.cpp`，完成 `QApplication` 启动与 metatype 注册。
   - 新增 `app/main_window.cpp`，完成：
     - UI 组件搭建（状态、遥测摘要、视频、日志、Start/Stop）
     - 三个 worker 的线程化生命周期管理
     - Start/Stop 控制链路
     - telemetry 与 frame 合流并投递给记录线程

2. **数据类型与记录模块增强**
   - `core/types.h` 增加 `Q_DECLARE_METATYPE(TelemetryPacket)`。
   - `media/record_worker.h` 增加 `Q_DECLARE_METATYPE(RecordItem)`。
   - `media/record_worker.cpp` 从简单文本占位升级为 `record_meta.jsonl` 抽样记录（每 15 帧），并输出延迟估计/检测/GPS字段。

3. **文档补齐**
   - 新增 `README.md`：项目说明、构建运行步骤、目录结构、线程解耦说明。
   - 新增 `PROGRESS.md`：记录本次开发、构建问题与修复。

### Configure / Build 结果
执行：

```bash
cmake -S . -B build
cmake --build build -j4
```

- Configure：成功。
- Build：首次失败后修复，最终成功。

### 构建错误与修复
**错误现象（首次 build）**
- `main_window.h` 中 `QVideoFrame` 未声明导致 MOC/编译阶段报错：
  - `‘QVideoFrame’ does not name a type`
  - 信号槽参数推导错误，出现 `const int&` 的级联报错。

**根因**
- `main_window.h` 缺少 `#include <QVideoFrame>`，MOC 无法解析槽函数参数类型。

**修复**
- 在 `src/client/app/main_window.h` 添加 `#include <QVideoFrame>`。
- 重新 build 通过，`sim_server` 与 `qt_client` 均成功生成。

### 当前状态
- 项目可配置、可编译。
- 客户端核心线程链路（TCP、RTSP、记录）已串通。
- 记录模块当前为元数据落盘（jsonl 抽样），未实现完整视频编码写盘。

### 下一步建议
1. 增加客户端配置项（host/port/rtsp/outDir）并持久化。
2. 为 `RecordWorker` 接入 `QMediaRecorder` 或 FFmpeg 管道，实现视频+元数据同步归档。
3. 增加断线重连策略（TCP/RTSP）与状态机。
4. 增加基础集成测试脚本（server + client smoke test）。


### Git 提交阶段问题与修复
**问题**
- 初次 `git commit` 失败：未配置提交身份（`user.name/user.email`）。

**修复**
- 在仓库内设置本地身份：
  - `git config user.name "OpenClaw Subagent"`
  - `git config user.email "subagent@local"`
- 重新提交成功。

