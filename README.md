# qt_rtsp_tcp_project

一个最小可运行的 **RTSP 视频 + TCP 遥测** 联动示例：

- `sim_server`（C++）：
  - 按 1Hz 在 TCP 端口推送 JSON telemetry（检测结果 + GPS）
  - 打印 FFmpeg 命令提示，便于将本地视频循环推送到 RTSP
- `qt_client`（Qt6）：
  - 独立线程接收 TCP telemetry
  - 独立线程拉取 RTSP 视频
  - 独立线程记录抽样后的融合元数据 `jsonl`
  - UI 显示实时状态、遥测摘要、视频画面与日志

## 目录结构

```text
src/
  common/                # 跨端公共协议与工具
  server/                # sim_server
  client/
    app/                 # Qt UI 入口与主窗口
    network/             # TcpClientWorker
    media/               # StreamWorker / RecordWorker
    core/                # 客户端类型定义
```

## 构建

```bash
cmake -S . -B build
cmake --build build -j
```

产物：

- `build/src/server/sim_server`
- `build/src/client/qt_client`

## 运行示例

### 1) 启动模拟服务端

```bash
./build/src/server/sim_server 9000 sample.mp4
```

服务端会打印 RTSP 推流建议命令（手动执行）：

```bash
ffmpeg -re -stream_loop -1 -i "sample.mp4" -an -c:v libx264 -preset veryfast -tune zerolatency \
  -f rtsp -rtsp_transport tcp "rtsp://127.0.0.1:8554/live"
```

> 需本地有可用 RTSP 服务（例如 mediamtx）监听 `:8554`。

### 2) 启动 Qt 客户端

```bash
./build/src/client/qt_client
```

点击 **Start** 后默认连接：

- TCP: `127.0.0.1:9000`
- RTSP: `rtsp://127.0.0.1:8554/live`

## 记录输出

客户端记录线程会抽样将融合元数据写到：

- `~/.local/share/qt_client/records/record_meta.jsonl`（Linux 默认）

字段包含：帧时间戳、是否有效帧、遥测延迟估计、检测信息、GPS 信息。

## 线程/模块解耦说明

- UI 线程仅负责展示与控制
- `TcpClientWorker`、`StreamWorker`、`RecordWorker` 分别运行在独立 `QThread`
- 线程间通过 Qt 信号槽 + `QueuedConnection` 传递数据
- 自定义跨线程消息 `TelemetryPacket` / `RecordItem` 通过 `Q_DECLARE_METATYPE + qRegisterMetaType` 注册

