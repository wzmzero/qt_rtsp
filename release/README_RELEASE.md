# Release 运行说明

## 目录约定（全部相对当前工作目录）
- `./data`：SQLite 数据库（`client_data.sqlite`）
- `./logs`：客户端日志文件（`qt_client.log`）
- `./snapshots`：截图
- `./recordings`：录制元数据（`record_meta.jsonl`）
- `./conf/client.ini`：配置文件

## 启动步骤
1. 进入发布目录：
```bash
cd release
```
2. 启动模拟服务端：
```bash
./scripts/run_server.sh
```
3. 新开终端，启动客户端：
```bash
cd release
./scripts/run_client.sh
```

## 手工启动（可选）
```bash
cd release
./bin/sim_server
./bin/qt_client
```

## 说明
- 配置文件建议保存相对路径（默认已如此），便于跨机器拷贝运行。
- person+rod 组合告警已启用基础实现：读取检测对象 `label/confidence/bbox`，按 IoU 阈值（默认 0.10）触发状态与日志。
