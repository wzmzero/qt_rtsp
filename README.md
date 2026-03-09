# qt_rtsp

一个用于“视频流 + 遥测消息”联调的演示工程：

- **sim_server（C++）**：
  - 通过 FFmpeg 把本地视频推到 RTSP
  - 通过 TCP 周期发送 Protobuf 遥测（检测目标 + GPS）
- **qt_client（Qt6）**：
  - 同时接收 RTSP 与 TCP
  - 展示连接状态、原始/解析数据
  - 基于 person+rod 规则做分级告警
  - 落库 SQLite，支持日志/事件查询
- **media_core（可选）**：独立的媒体能力封装/测试目标

---

## 1. 适用场景

这个项目适合以下用途：

- 客户端联调：在没有真实设备时，先用模拟端打通协议与 UI。
- 协议验证：验证 Protobuf 帧边界、字段映射、时间戳语义。
- 告警策略验证：在固定消息或 YOLO 标签输入下复现实验。
- 数据链路排障：从 RTSP、TCP 到 UI、DB 全链路定位问题。

---

## 2. 总体架构

数据链路如下：

1) `sim_server` 从 `media/test.mp4` 推 RTSP 到 `rtsp://127.0.0.1:8554/live`
2) `sim_server` 同时在 TCP 端口（默认 `9000`）发送 Protobuf 帧
3) `qt_client`：
   - `stream_worker` 播放 RTSP
   - `tcp_client_worker` 收包并解析 `Telemetry`
   - `main_window` 聚合显示并执行业务规则
   - `database_service` 异步落库

协议采用 **4字节大端长度 + protobuf payload**。

---

## 3. 目录结构

- `src/common`
  - `msg/message.proto`：协议定义
  - `protocol/messages.h`：服务端组包工具
- `src/server`
  - `network/tcp_sim_server.*`：TCP 模拟发送器
  - `media/rtsp_launcher.*`：FFmpeg 推流控制
  - `app/main.cpp`：参数解析、双 supervisor 管理
- `src/client`
  - `network/tcp_client_worker.*`：TCP 收包/解码
  - `media/stream_worker.*`：RTSP 播放
  - `media/record_worker.*`：录制相关
  - `app/main_window.*`：UI 与告警逻辑
  - `database/database_service.*`：SQLite 访问与迁移
  - `repository/app_repository.*`：应用层仓储抽象
- `config/`：默认数据库与配置目录
- `labels/`：YOLO 标签示例与类别映射
- `media/`：测试视频与录制数据

---

## 4. 关键设计说明

## 4.1 协议模型（message.proto）

顶层消息 `Telemetry` 包含：

- `sent_ts_ms`：发送时刻（服务器发送时间）
- `source_ts_ms`：源数据产生时刻（检测时间）
- `detections[]`：多目标检测（label_id/label/confidence/cx/cy/w/h）
- `gps`：定位信息（E7 坐标、速度、航向、fix_type 等）

设计重点：

- 支持多目标（repeated），不仅是单目标。
- bbox 用归一化中心点格式，便于跨分辨率渲染。
- 时间戳分离 source/sent，便于评估链路延迟。

## 4.2 服务端稳定性设计

`sim_server` 内部将 TCP 与 RTSP 拆分为两个 supervisor 线程：

- 任一子系统异常不会直接拖垮主进程。
- TCP 失败采用固定间隔重试。
- RTSP 失败采用指数退避重试。
- 支持 `--require-rtsp` 模式以增强部署约束。

## 4.3 客户端告警规则

当前规则：同一条 Telemetry 中同时存在 person 与 rod，且两者置信度都超过阈值即触发。

- 低/中/高三级阈值（默认 `0.5/0.7/0.85`）
- 阈值可配置并持久化
- 不再依赖 IoU 计算，规则更直观

## 4.4 数据持久化

SQLite 默认路径：`./config/client_cfg.db`

核心表：

- `app_config`：单行应用配置（连接、主题、窗口状态等）
- `telemetry_results`：遥测主记录
- `gps_samples`：GPS 明细
- `snapshot_events`：截图事件
- `app_logs`：应用日志
- `playback_index`：回放索引
- `schema_version`：版本迁移记录

包含旧库迁移逻辑，兼容历史结构。

---

## 5. 编译与依赖

## 5.1 依赖

- CMake >= 3.16
- C++17 编译器
- Protobuf（libprotobuf）
- Qt6（Core/Widgets/Network/Multimedia/MultimediaWidgets/Sql）
- FFmpeg（运行 sim_server 推流需要）

## 5.2 关于本次编译失败（重点）

你遇到的错误：

`Protobuf_PROTOC_EXECUTABLE-NOTFOUND`

根因是：系统找到了 `libprotobuf`，但没找到 `protoc`（代码生成器）。

本仓库已支持优先查找：

- 系统 `protoc`
- 项目本地 `./.tools/bin/protoc`

如果缺失会在 CMake 配置阶段直接报错，避免编译到 4% 才失败。

## 5.3 处理方案

方案 A（推荐）：安装系统 protoc

```bash
# Ubuntu/Debian
sudo apt-get install -y protobuf-compiler
```

方案 B（无 sudo）：放置本地 protoc

```bash
mkdir -p .tools
# 将 protoc 二进制放到 ./.tools/bin/protoc 并 chmod +x
```

## 5.4 构建命令

```bash
cmake -S . -B build
cmake --build build -j4
```

可选：若仅先验证服务端，可关闭客户端：

```bash
cmake -S . -B build -DBUILD_CLIENT=OFF
cmake --build build -j4
```

---

## 6. 运行说明

### 6.1 启动 RTSP 服务（例如 mediamtx）

```bash
mediamtx
```

### 6.2 启动模拟服务端

```bash
./build/src/server/sim_server \
  --tcp-port 9000 \
  --video media/test.mp4 \
  --rtsp-url rtsp://127.0.0.1:8554/live
```

可选参数（常用）：

- `--fixed-msg`：发送固定检测参数
- `--yolo-txt labels/example_labels.txt`：从标签文件读检测结果
- `--class-map labels/class_map.ini`：类别映射
- `--tcp-retry-ms`：TCP 重试间隔
- `--rtsp-retry-min-ms/--rtsp-retry-max-ms`：RTSP 退避区间

### 6.3 启动客户端

```bash
./build/src/client/qt_client
```

---

## 7. 日志与排障

- 客户端日志：`./logs/qt_client.log`
- FFmpeg stderr：`./logs/qt_ffmpeg_stderr.log`
- DB 文件：`./config/client_cfg.db`

常见问题：

1) **编译失败：protoc not found**
   - 检查 `which protoc`
   - 或检查 `./.tools/bin/protoc` 是否存在且可执行

2) **客户端能打开但无视频**
   - 检查 mediamtx 是否运行
   - 检查 RTSP URL 是否一致
   - 查看 `qt_ffmpeg_stderr.log`

3) **有视频但无遥测**
   - 检查 TCP 端口占用
   - 用 `nc` 或日志确认 sim_server 在发包

4) **有遥测但告警不触发**
   - 确认同一帧同时包含 person 和 rod
   - 检查阈值配置是否过高

---

## 8. 后续建议

- 增加端到端回放工具：DB + 视频时间线统一回放
- 增加协议兼容测试：字段新增/缺失回归
- 增加 CI：无头环境下自动编译 + 基础冒烟
- 提供 Docker 开发环境，统一依赖

