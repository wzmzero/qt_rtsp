# PROGRESS

## 2026-03-06（UI升级 + 可配置化 + 数据库接口层）

### 任务目标
1. Qt 客户端 UI 完整重构与美化。
2. 参数可配置并持久化（QSettings + SQLite）。
3. 新增 DatabaseService/Repository 抽象，UI 不直接写 SQL。
4. 新增检测结果/GPS 持久化。
5. 建立 schema_version + 初始化迁移入口。
6. 维持线程解耦，数据库写入不阻塞 UI。
7. 完成 configure/build 验证并提交。

### 实施阶段

#### 阶段A：配置对象与基础重构
- 新增 `core/app_config.h`：统一配置 DTO（RTSP/TCP/重连/录制/主题/窗口布局）。
- MainWindow 改为基于配置对象启动 worker。

#### 阶段B：数据库接口层
- 新增 `database/database_service.*`
  - `IDatabaseService` 抽象
  - `SQLiteDatabaseService` 实现
  - `initialize()` + `migrate()`（v1）
  - `loadConfig()` / `saveConfigAsync()` / `insertTelemetryAsync()`
- 新增 `repository/app_repository.*`
  - `IAppRepository` 抽象
  - `AppRepository` 实现
  - QSettings 与 SQLite 双写配置
  - telemetry 异步入库

#### 阶段C：UI升级与可配置化
- `main_window.*` 重写：
  - 顶部菜单栏（文件/连接/视图/帮助）
  - ToolBar + StatusBar
  - GroupBox 分区布局
  - 视频主区域居中、占主要空间
  - 状态面板显示连接状态/时间戳/检测/GPS
  - 样式表支持 dark/light 主题
- 参数输入：RTSP、TCP Host/Port、重连间隔、录制目录/开关、主题

#### 阶段D：线程解耦增强
- 保持 `TcpClientWorker` / `StreamWorker` / `RecordWorker` 独立线程。
- 新增 `dbThread`，数据库写入全部 `QueuedConnection` 投递。
- UI 线程仅负责状态编排与展示。
- `TcpClientWorker` 增加断线重连（可配置重连间隔）。

### 构建错误与修复
1. **首次 build 大量 Qt Core 头文件级联错误（`qcompare.h`）**
   - 根因：`core/app_config.h` 使用 `Q_DECLARE_METATYPE` 但缺少 `QMetaType` / `QtGlobal` 相关包含，导致 MOC/编译期语义异常。
   - 修复：在 `app_config.h` 增加 `#include <QMetaType>` 与 `#include <QtGlobal>`。

2. **`QCheckBox/QComboBox` 不完整类型报错**
   - 根因：`main_window.cpp` 中使用了成员函数，但未包含完整头文件。
   - 修复：补充 `#include <QCheckBox>`、`#include <QComboBox>`。

### Configure / Build 验证
执行命令：
```bash
cmake -S . -B build
cmake --build build -j4
```

结果：
- Configure：成功（存在 server 目标 AUTOGEN dev warning，不影响构建）。
- Build：首次失败后修复，最终成功。
- 产物：
  - `build/src/server/sim_server`
  - `build/src/client/qt_client`

### 变更文件
- `src/client/core/app_config.h`
- `src/client/database/database_service.h`
- `src/client/database/database_service.cpp`
- `src/client/repository/app_repository.h`
- `src/client/repository/app_repository.cpp`
- `src/client/app/main_window.h`
- `src/client/app/main_window.cpp`
- `src/client/app/main.cpp`
- `src/client/network/tcp_client_worker.h`
- `src/client/network/tcp_client_worker.cpp`
- `src/client/CMakeLists.txt`
- `README.md`
- `PROGRESS.md`
