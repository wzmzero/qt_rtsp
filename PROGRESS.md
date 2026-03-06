# PROGRESS

## 2026-03-06（UI 重构 + 参数可配置化，仅 QSettings）

### 目标
1. 完成 Qt 客户端界面重构。
2. 参数编辑区覆盖 RTSP/TCP/端口/重连/录制路径。
3. 使用 QSettings 做配置持久化。
4. 保持线程解耦模型不变（TCP / RTSP / Record）。
5. 不新增数据库能力。

### 实施内容

#### 1) 主界面结构调整
- 新增/完善：菜单栏（文件/连接/视图/帮助）、工具栏、状态栏。
- 中央主区改为 `QSplitter`：
  - 左侧：主视频区（居中、占主区）
  - 右侧：状态面板 + 参数编辑区
- 底部保留日志输出区。

#### 2) 参数编辑区与交互
- 参数项：
  - RTSP URL
  - TCP Host
  - TCP Port
  - 重连间隔(ms)
  - 录制路径（支持目录选择）
  - 录制开关
  - 主题切换（dark/light）
- 支持按钮：启动、停止、保存配置。

#### 3) 配置持久化
- 通过 `QSettings` 统一读写：
  - 连接参数、录制参数、主题
  - `window_geometry` / `window_state`
- 启动自动加载、保存即时落盘（`sync()`）。

#### 4) 线程模型
- 保持原有解耦：
  - `TcpClientWorker` 在线程 `tcpThread_`
  - `StreamWorker` 在线程 `streamThread_`
  - `RecordWorker` 在线程 `recordThread_`
- 线程启动/停止流程维持不变。

### 变更文件
- `src/client/app/main_window.h`
- `src/client/app/main_window.cpp`
- `src/client/CMakeLists.txt`
- `README.md`
- `PROGRESS.md`

### Configure / Build 验证
执行：
```bash
cmake -S . -B build
cmake --build build -j4
```

结果：通过（见本次构建日志）。
