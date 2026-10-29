# 交通灯实验

## 打开和运行

1. 用 Keil µVision 打开 `../../jiaotongdeng/jiaotongdeng.uvproj`，按 F7 编译。
2. 用 Proteus 打开本目录的 `jiaotongdeng.pdsprj`，点击左下角 ▶ 运行。
3. 修改代码后先停止仿真，再重新编译并启动仿真。

已附编译好的 HEX，可直接启动仿真。U1 的 Program File 使用相对路径 `../../jiaotongdeng/Objects/jiaotongdeng.hex`；移动工程时请一起保留 `jiaotongdeng` 和 `fangzhen` 两个目录的相对位置。

## 原理图

沿用流水灯工程的 AT89C51 最小系统，晶振和 MCU 时钟均为 **12 MHz**，标准 12T。六路均按 `VCC → 680 Ω → LED 阳极 → LED 阴极 → P1` 连接，**低电平点亮**。每个 LED 代表对应方向的一组交通灯。NS 表示南北，EW 表示东西。

| IO | LED | 标注 | 含义 |
|---|---|---|---|
| P1.0 | D1 | NS_GREEN | 南北绿灯 |
| P1.1 | D2 | NS_YELLOW | 南北黄灯 |
| P1.2 | D3 | NS_RED | 南北红灯 |
| P1.3 | D4 | EW_GREEN | 东西绿灯 |
| P1.4 | D5 | EW_YELLOW | 东西黄灯 |
| P1.5 | D6 | EW_RED | 东西红灯 |

`schematic.png` 为原理图预览，`schematic.svg` 为可缩放版本；实际可编辑电路保存在 `.pdsprj` 文件中。

## 程序和状态

`main.c` 使用程序存储器中的状态表和时长表，按表整字节输出 P1，并调用软件延时子程序。保留与流水灯工程一致的简单文件结构。

| 状态 | 南北 | 东西 | P1 输出 | 持续时间 |
|---|---|---|---|---|
| 1 | 绿 | 红 | 0xDE | 约 1.5 秒 |
| 2 | 黄 | 红 | 0xDD | 约 0.5 秒 |
| 3 | 红 | 绿 | 0xF3 | 约 1.5 秒 |
| 4 | 红 | 黄 | 0xEB | 约 0.5 秒 |

四状态循环一次约 4 秒。延时参数针对本工程的 Keil 编译设置和 12 MHz 时钟校准；改变时钟或优化设置后需要重新核对。

## 已完成的验证

- Keil C51 编译：0 Error、0 Warning，代码 104 字节，数据 9 字节。编译日志在 `../../jiaotongdeng/build.log`。
- Proteus 实际加载 HEX 并运行，原生 MCP 采集 307 次 LED 状态，覆盖两个完整循环。
- 观测四阶段顺序及持续时间为 1.5、0.5、1.5、0.5 秒；采样中未发现无效组合或两个方向同时亮绿灯。时间以仿真界面的更新精度为准。
- `verification.json` 保存状态变化时间；`simulation.log` 保存仿真日志；`netlist.json` 保存电气连接供核对。
