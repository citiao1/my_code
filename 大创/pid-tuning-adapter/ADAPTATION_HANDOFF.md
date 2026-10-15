# PID 整定适配交接说明

本文档供后续 Codex 对话继续把开源 PID 整定工具适配到当前 BM50 四轮速度环项目。目标是让后续对话能先理解现状，再针对已选定的整定工具实现具体格式/API 对接；不要将目前的数据采集器误认为已包含自动整定算法。

## 1. 当前结论

- 电机板为 STM32F407VGT6，项目目录：`vgt6_module_test/`。
- 当前上位机目录：`mecanum-motor-console/`。浏览器页面通过本地 Python WebSocket 桥接通信；桥接进程是 BLE Central 和硬件命令的唯一所有者。
- 当前固件版本标识为 `VGT6_BM50_SPEED_TUNABLE_R6`；桥接器仍兼容只读的
  `VGT6_BM50_SPEED_FIXED_R5`。
- 四轮速度环默认增益为 `Kp=1.2, Ki=0.2, Kd=0`。R6 的 `PIDGET` 读取四轮增益，
  `PID,<轮>,Kp,Ki,Kd` 接受有限的 0..100 参数；在线写入由外部整定器的会话锁和
  回读校验保护。
- 已新增独立的 `pid-tuning-adapter/` Python 工具。它通过现有 WebSocket 读取 TEL 并写出单轮 CSV，可供后续离线系统辨识/整定使用；它不是控制器，也没有与任何一个具体第三方整定器完成格式/API 对接。
- 之前的 GitHub 搜索结果、选定仓库地址和版本没有记录在当前工作区文档中。后续对话应先让用户提供仓库/项目名称或重新核实候选，不能臆测一个已选定项目。

## 2. 相关代码位置

| 用途 | 文件 |
| --- | --- |
| 本适配器、WebSocket 请求与遥测 CSV 解析 | `pid-tuning-adapter/pid_ws_logger.py` |
| 本适配器测试 | `pid-tuning-adapter/test_pid_ws_logger.py` |
| 本适配器部署方式及字段简表 | `pid-tuning-adapter/README.md` |
| 上位机浏览器的 JSON 请求、结果关联和 TEL 处理 | `mecanum-motor-console/touch-remote.js` |
| WebSocket、命令队列、MCU ACK 匹配和硬件转发 | `mecanum-motor-console/bridge/motor_vofa_bridge.py` |
| MCU 命令解析、PID 范围校验、PIDGET、SPEEDLIMIT、SPD、SPEED、TEL 控制 | `vgt6_module_test/App/Src/command.c` |
| 27 字段速度遥测定义 | `vgt6_module_test/App/Src/telemetry.c` |
| 速度环运行、目标上限与输出实现 | `vgt6_module_test/App/Src/speed_control.c` |
| 速度环默认参数和 PID 实例 | `vgt6_module_test/App/Src/speed_control.c` |
| 项目现有命令和硬件测试说明 | `vgt6_module_test/README.md`、`mecanum-motor-console/README.md` |

代码行号会随后续编辑变化；开始改动前请重新检查这些文件中的当前实现。

## 3. WebSocket 接口

- 默认地址：`ws://127.0.0.1:8766`。
- 连接后先收到桥接能力标识 `BRIDGE,BM50_ACK_R3`，以及硬件 `STATUS`。
- 新接口请求为一条 JSON：

  ```json
  {"type":"command","id":1,"command":"PIDGET"}
  ```

- 桥接返回关联同一 `id` 的 JSON 结果，例如：

  ```json
  {"type":"result","id":1,"ok":true,"reply":"PID,1.2000,0.2000,0.0000,..."}
  ```

- MCU 原始应答和 TEL 仍会以普通文本消息广播给所有 WebSocket 客户端。消费者应忽略不属于本请求的文本消息，并用 JSON `id` 关联命令结果。
- 桥接会串行执行 MCU 命令并等待对应 ACK/查询结果，单条命令等待超时为 3 秒；前端结果等待超时为 5 秒。硬件命令不能因为超时而自动重试。
- WebSocket 断开时，若它是最后一个客户端，桥接会发送 `STOP`。运行采集期间应保持上位机页面连接；不要启动第二个桥接进程或由适配器直接打开 BLE/串口。

## 4. MCU 命令边界

常用只读/状态命令：

- `VER`：固件版本。
- `STACK`：栈分配、已用水位、剩余空间、保护状态。上位机要求分配 4096 B、剩余至少 32 B、数值自洽且保护位正常。
- `LINK`：UART/命令统计和周期遥测状态；末字段 `1` 表示 TEL 周期流开启。
- `PIDGET`：四轮各自的 Kp/Ki/Kd。
- `SPEEDSTATUS`：单次 TEL。
- `TEL,1` / `TEL,0`：开启/关闭 300 ms 周期 TEL。该操作不启停电机，但会改变固件遥测状态。

控制命令包括 `ARM`、`SPEED,0/1`、`SPD,<A-D>,<rpm>`、`DRV,...`、`STOP` 等。当前采集器不发送这些命令。目标上限由固件约束，当前项目目标上限为 200 RPM。真正的采样实验由操作者通过现有上位机发出保守目标阶跃。

PID 写入现状必须遵守：

- 固件 PID 命令语法为 `PID,<A-D>,<Kp>,<Ki>,<Kd>`。
- R5 命令实现只把匹配固定参数的请求作为兼容 no-op；R6 返回 `ACK,PID` 前会做 0..100
  有限值校验。
- WebSocket 返回 `ok:true` 只代表 MCU 对该命令返回了非错误应答，不能代替 `PIDGET` 回读。
- R6 在线整定还必须使用外部 bridge 的 live session：首次人工确认后锁定目标、
  `SPEEDLIMIT`、输出限幅和通道，轮次间不停车；异常时发送 `STOP`。

## 5. TEL 字段到 CSV 的映射

固件以 300 ms 周期输出 27 个逗号分隔字段。以 `TEL` 为字段 0：

| TEL 字段 | 内容 | CSV/说明 |
| --- | --- | --- |
| 1 | HAL tick，毫秒 | `time_s`：以首个采样 tick 为零点换算秒 |
| 2 | 当前保留/固定字段 | 忽略 |
| 3 | 速度环启用标志 | 只在值为 `1` 时记入 CSV |
| 4–7 | A/B/C/D 目标 RPM | `setpoint_rpm` 对应 `4 + 轮索引` |
| 8–11 | A/B/C/D 输出百分比 | `output_percent` 对应 `8 + 轮索引` |
| 12–15 | A/B/C/D 实际 RPM ×10 | `process_rpm` 对应 `12 + 轮索引` 后除以 10 |
| 16–19 | 目标 RPM 副本 | 当前 CSV 不使用 |
| 20–26 | 电池、陀螺仪和状态字段 | 当前 CSV 不使用 |

注意：索引表使用人类可读的 TEL 字段号（从 `TEL` 起算）。在 Python `split(",")` 列表中索引与字段号相同，因为 `TEL` 是索引 0。

当前 CSV 列：

```text
time_s,setpoint_rpm,process_rpm,output_percent
```

整定库可能要求不同的列名、时间单位、控制器输出格式或实验元数据。适配时应在 CSV 导出层转换，不应因此改变 MCU 协议或原始遥测语义。

## 6. 已部署工具

目录：`pid-tuning-adapter/`

- `pid_ws_logger.py`：连接 WebSocket，验证 `VER`、`STACK`、`LINK`、`PIDGET`；若周期 TEL 原先关闭则临时开启；记录所选轮样本；正常退出时尝试恢复原遥测状态。
- `requirements.txt`：`websockets>=12,<16`。
- 本机隔离环境已建立在 `pid-tuning-adapter/.venv/`，当前使用 `websockets 15.0.1`；该目录由本目录 `.gitignore` 排除。
- `test_pid_ws_logger.py`：覆盖正确轮索引/单位换算、相对时间、速度环关闭和畸形数据过滤。

从仓库根目录启动：

```powershell
pid-tuning-adapter\.venv\Scripts\python.exe pid-tuning-adapter\pid_ws_logger.py --motor A --output pid-tuning-adapter\capture-A.csv
```

将 `A` 替换为 `B`、`C` 或 `D`。结束时按 Ctrl+C。此前验证：适配器单测 2 项通过，Python `py_compile` 通过。尚未连接实体设备跑完整 BLE/WebSocket/CSV 实测。

## 7. 实验与安全前置条件

1. 核实轮子离地、机械结构稳定、急停/断电方式可用，再启用速度环。
2. 操作者通过原上位机设置低目标值并发出阶跃；记录需覆盖阶跃前稳定段、上升过程和稳态段。
3. 不要让离线整定器自动修改目标、ARM 状态或 PID 参数。若未来要做闭环自动实验，必须先单独完成风险评估和人工授权。
4. 观察输出百分比是否长时间贴近限制；输出饱和的数据不能直接当成未饱和线性对象拟合。
5. `TEL` 频率约 3.33 Hz，速度控制周期为 10 ms。该采样率不足以呈现每个控制周期的瞬态；整定算法必须适合低频记录，或者先经评估调整遥测采样/数据采集方案。不要误把 TEL 采样周期当作 PID 控制周期。
6. BLE UART 链路实际波特率和命令长度限制需与当前固件/桥接一致；不得因增加采集数据而挤占 ACK 带宽或破坏 LF/CRLF 分帧。
7. 本文档和当前工具仅提供离线数据采集/候选建议边界，不构成实体电机参数安全认证。

## 8. 下一对话建议流程

1. 先询问或重新查证用户选定的 GitHub PID 整定项目、提交/版本、运行方式和输入数据格式；目前本目录未保存这些信息。
2. 检查该项目能否使用当前 CSV 离线拟合；确认它需要的目标值、实际值、输出值、采样间隔、单位及预处理。
3. 若只需离线输入转换，在 `pid-tuning-adapter/` 内增加独立转换/调用模块和测试，保持 MCU、桥接、浏览器现有代码不动。
4. 若用户明确要求在线闭环调参，先拆分为“采集”“参数建议”“人工确认”“MCU 写入”阶段；在可回读、范围约束、失败回滚和台架验收未完成前，只实现前三阶段。
5. 如确需改 MCU/桥接/上位机，逐项列出改动和原因；保留当前工作区已有改动，不做无关清理或回滚。
6. 至少运行适配器单测，以及受改动模块对应的测试：

   ```powershell
   pid-tuning-adapter\.venv\Scripts\python.exe -m unittest discover -s pid-tuning-adapter -p "test_*.py" -v
   node --test mecanum-motor-console/tests/touch-remote.test.cjs
   mecanum-motor-console\bridge\.venv\Scripts\python.exe -m unittest discover -s mecanum-motor-console\bridge -p "test_*.py" -v
   vgt6_module_test\tests\host\run-tests.ps1
   ```

## 9. 当前工作区注意事项

仓库不是干净状态，`mecanum-motor-console/` 与 `vgt6_module_test/` 有大量既存修改、删除和新增文件，来源可能包含用户先前工作。后续对话开始前先运行 `git status --short`，将这些状态视为用户数据；除非用户明确要求，不要清理、恢复或覆盖它们。本次新增的适配文件集中在 `pid-tuning-adapter/`。
