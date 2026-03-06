# qt_rtsp_tcp_project

Qt6 客户端 + C++ 模拟服务端示例，支持 RTSP 视频与 TCP 遥测融合展示。
本次改造聚焦 **UI 重构 + 参数可配置化（QSettings）**，不新增数据库能力。

## 本次实现（UI + 配置）

- 菜单栏 + 工具栏 + 状态栏
- 主视频区域居中并占据主工作区
- 右侧状态面板（连接状态 / 时间戳 / 检测结果 / GPS）
- 参数编辑区（可直接改并保存）：
  - RTSP URL
  - TCP Host
  - TCP Port
  - 重连间隔(ms)
  - 录制路径
  - 录制开关
- 使用 `QSettings` 持久化配置，启动自动加载，保存后立即生效
- 保持 TCP / RTSP / Record 三线程解耦模型不变

## 目录（客户端）

```text
src/client/
  app/                  # MainWindow(UI编排)
  core/                 # DTO + AppConfig
  network/              # TcpClientWorker
  media/                # StreamWorker / RecordWorker
```

## 构建

```bash
cmake -S . -B build
cmake --build build -j4
```

## 运行

### 1) 启动模拟服务端

```bash
./build/src/server/sim_server 9000 sample.mp4
```

### 2) 启动客户端

```bash
./build/src/client/qt_client
```

## 配置持久化说明

客户端使用 `QSettings` 保存参数（组织名 `qt_rtsp_tcp_project`，应用名 `qt_client`）。
包含连接参数、录制参数、主题与窗口布局（geometry/state）。

常见 Linux 存储位置通常位于：
- `~/.config/qt_rtsp_tcp_project/qt_client.conf`（取决于 Qt 平台后端）
