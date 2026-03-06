# PROGRESS

## 2026-03-06（服务端一体化：sim_server 自动推流 + TCP 并行）

### 目标
1. `sim_server` 启动时自动拉起推流进程（默认 `media/test.mp4 -> rtsp://127.0.0.1:8554/live`）。
2. 去掉“手动终端 C”要求，保留视频路径/RTSP URL/ffmpeg 路径可配置。
3. 服务端仅使用标准库/系统调用，不引入 Qt 依赖。
4. 推流失败时给出明确错误，并支持失败策略配置（继续 TCP 或直接退出）。
5. 更新文档，给出新启动流程（A+B+D），并完成 configure/build 验证。

### 实施内容

#### 1) RTSP 推流进程管理（server 侧）
- 重构 `RtspLauncher`：由“仅打印命令”升级为真正的进程管理器。
- 新增能力：
  - `start()`：通过 `fork + execvp` 启动 ffmpeg 推流进程。
  - `stop()`：通过 `SIGTERM` + `waitpid` 联动停止；超时后 `SIGKILL` 兜底。
  - `ffmpeg_command()`：用于日志打印实际启动命令。
- 启动健壮性：
  - 启动后短窗口内轮询 `waitpid(WNOHANG)`，检测 ffmpeg 是否“秒退”。
  - 秒退时输出明确错误（退出码/信号 + 路径配置提示）。

#### 2) sim_server 参数与策略
- 新增命令行参数：
  - `--tcp-port <port>`（默认 9000）
  - `--video <path>`（默认 `media/test.mp4`）
  - `--rtsp-url <url>`（默认 `rtsp://127.0.0.1:8554/live`）
  - `--ffmpeg <path>`（默认 `ffmpeg`）
  - `--require-rtsp`（推流失败即退出）
- 默认策略：推流失败打印 WARN，TCP 模拟服务继续运行。
- 严格策略：`--require-rtsp` 下，推流失败返回非零退出。
- 保留旧兼容位置参数：`sim_server [tcp_port] [video_path]`。

#### 3) 生命周期联动
- `SIGINT/SIGTERM` 时同时触发：
  - TCP 服务 `stop()`
  - RTSP 推流 `stop()`
- 主循环退出后再次执行 RTSP 停止，确保回收子进程。

#### 4) 文档更新
- `README.md` 更新为新流程：A+B+D（不再需要手动终端 C）。
- 增补参数说明、默认值、失败策略说明、运行示例与兼容性说明。

### 变更文件
- `src/server/media/rtsp_launcher.h`
- `src/server/media/rtsp_launcher.cpp`
- `src/server/app/main.cpp`
- `README.md`
- `PROGRESS.md`

### Configure / Build 验证
执行：
```bash
cmake -S . -B build
cmake --build build -j4
```

结果：通过。

### 运行示例

```bash
# 默认（自动推流 + TCP）
./build/src/server/sim_server

# 自定义视频/RTSP/ffmpeg
./build/src/server/sim_server \
  --tcp-port 9100 \
  --video media/test.mp4 \
  --rtsp-url rtsp://127.0.0.1:8554/live \
  --ffmpeg /usr/bin/ffmpeg

# 推流失败即退出
./build/src/server/sim_server --require-rtsp
```

### 已知限制
1. 进程管理基于 POSIX 系统调用（Windows 原生需额外适配）。
2. “启动成功”判定为“未在短时间内秒退”，不等价于 RTSP 链路已被下游成功消费。
3. 当前未做 ffmpeg stdout/stderr 管道采集，细节日志由 ffmpeg 进程直接输出到控制台。
