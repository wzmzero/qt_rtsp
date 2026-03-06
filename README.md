# qt_rtsp_tcp_project

Qt6 客户端 + C++ 模拟服务端示例，支持 RTSP 视频与 TCP 遥测融合展示。  
当前服务端已支持“一体化启动”：`sim_server` 启动时自动拉起 ffmpeg 推流进程，并在退出时联动停止。

## 本次实现（服务端一体化 + 客户端配置）

- 客户端 UI 重构 + `QSettings` 参数持久化（原有）
- 服务端 `sim_server` 自动启动 RTSP 推流（不再需要手动终端 C）
- 服务端推流参数可配置：
  - 视频路径（`--video`）
  - RTSP URL（`--rtsp-url`）
  - ffmpeg 路径（`--ffmpeg`）
- 推流失败健壮策略：
  - 默认：推流失败仅告警，TCP 模拟服务继续运行
  - 可选：`--require-rtsp`，推流失败即退出
- 服务端仅使用标准库 + 系统调用（`fork/execvp/kill/waitpid`），不依赖 Qt

## 构建

```bash
cmake -S . -B build
cmake --build build -j4
```

## 运行（新的 A+B+D）

### A) 启动 RTSP 服务端（例如 mediamtx）

```bash
mediamtx
```

### B) 启动 sim_server（自动推流 + TCP 模拟）

```bash
./build/src/server/sim_server
```

默认等价于：

```bash
./build/src/server/sim_server \
  --tcp-port 9000 \
  --video media/test.mp4 \
  --rtsp-url rtsp://127.0.0.1:8554/live \
  --ffmpeg ffmpeg
```

常用参数：

```bash
./build/src/server/sim_server \
  --tcp-port 9100 \
  --video /path/to/demo.mp4 \
  --rtsp-url rtsp://127.0.0.1:8554/live \
  --ffmpeg /usr/bin/ffmpeg
```

如果希望“推流必须成功”，加：

```bash
./build/src/server/sim_server --require-rtsp
```

### D) 启动客户端

```bash
./build/src/client/qt_client
```

## 配置持久化说明（客户端）

客户端使用 `QSettings` 保存参数（组织名 `qt_rtsp_tcp_project`，应用名 `qt_client`）。  
包含连接参数、录制参数、主题与窗口布局（geometry/state）。

常见 Linux 存储位置通常位于：
- `~/.config/qt_rtsp_tcp_project/qt_client.conf`（取决于 Qt 平台后端）

## 兼容性说明

- `sim_server` 仍兼容旧位置参数写法：
  - `./build/src/server/sim_server [tcp_port] [video_path]`
- 当前进程管理实现基于 POSIX（Linux/macOS 风格系统调用）。
