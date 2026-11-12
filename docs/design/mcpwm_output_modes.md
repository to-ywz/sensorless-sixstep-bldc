# LKS32MC071 MCPWM 输出模式核查

> **2026-11-09极性修复优先说明：** 本板LKS560为HIN高有效、LIN低有效，物理关断P低/N高。旧文中的六路低及高有效互补测量描述保留作历史说明；当前A1～A3将N输出反相，A4使用物理电平表。CH_DEF绕过普通极性，默认N必须单独设高。见 `tests/reports/2026-11-09_FIX-LKS560-polarity.md`。

适用硬件：LKS32MC071CBT8，当前项目使用官方标准外设驱动库 V3.16。

## 1. 先区分两个概念

- **中心对齐互补输出（Complementary）**：同一 PWM 周期内，P、N 两路按比较阈值切换；换向时硬件插入死区。用于半桥上下管控制。
- **中心对齐推挽交替输出（Push-pull）**：第一个周期由 P 输出，第二个周期由 N 输出，之后交替。P 或 N 单独观察时，其脉冲重复频率为基准频率的一半。

这里的“推挽”不是 GPIO 的 push-pull 电气输出类型，而是 MCPWM 的 P/N 周期交替模式。

## 2. 当前 SDK 的配置对应关系

### 时间基准与对齐方式

- `MCPWM0_CENTRAL_PWM_MODE = 0`：中心对齐。
- `MCPWM0_EDGE_PWM_MODE = 1`：边沿对齐。
- 标准驱动 `MCPWM_Init()` 将 `MCPWM_WorkModeCH0` 写入 `MCPWM0_IO01` 的 CH0 工作模式位。
- 中心对齐模式下，计数器从 `-TH0` 计数到 `+TH0`，周期为 `(2 × TH0 + 1) / f_MCPWM`。

### P/N 输出源选择

- `CH0_PS = 0`、`CH0_NS = 0`：P/N 均由 MCPWM 硬件波形驱动。
- `CH0_PS = 1` 或 `CH0_NS = 1`：对应输出由 `CH0P_SCTRLP` / `CH0N_SCTRLN` 软件固定电平控制。
- A1 为了只测 CH0P，设置了 `CH0_NS = 1`、`CH0N_SCTRLN = LOW`。A2 必须取消这个静态覆盖，让 `CH0_NS = 0`。

### 推挽交替

- `IO0_PPE` 对应 MCPWM0_PP 的 IO0 推挽使能。
- A1 已通过实测确认：`IO0_PPE = ENABLE` 时只观察 CH0P，实测约 5.036kHz、12.6%；改为 DISABLE 后实测约 10.086kHz、24.8%。
- A2 继续保持 `IO0_PPE = DISABLE`，不启用推挽交替。

## 3. 互补输出与死区时序

中心对齐互补模式下，手册描述的两个关键事件为：

1. CNT 向上计数到 `TH00`：关闭 CH0N，等待 DTH0，然后打开 CH0P。
2. CNT 向上计数到 `TH01`：关闭 CH0P，等待 DTH1，然后打开 CH0N。

因此两个换向间隔都应出现 P/N 同时为低的死区；正常情况下不允许 P/N 同时为高。

## 4. 死区时钟换算与 A2 台架值

HAL 驱动源码中的 `HAL_MCPWM_GetClockFreq()` 使用：

`f_deadtime = HAL_CLK_GetMcuFreq() / MCPWM时钟分频系数`

`HAL_MCPWM_SetDead()` 再按 `deadtime_count = deadtime_ns × f_deadtime / 1e9` 将纳秒换算成计数值。该 HAL 源文件当前**没有加入本项目 Keil 主目标**；A2 仍使用主目标已经在用的标准外设库 `MCPWM_Init()`，通过 `DeadTimeCH012N/P` 写入原始计数值。

A1 实测载波约 10.086kHz，TH0=2400，对应反推计数时钟约：

`10.086kHz × (2 × 2400 + 1) ≈ 48.42MHz`

A2 仅作 MCU 逻辑波形观察，初始死区设为 **48 个 MCPWM 时钟计数**，估算：

`48 / 48.42MHz ≈ 0.99µs`

这是为了在示波器上清楚观察死区的台架值，**不是功率级最终死区设计值**。最终值必须结合实际 MOSFET/IGBT、预驱传播延迟、关断尾电流、温度和实测栅极波形重新确定。

## 5. 死区寄存器分组注意事项

当前标准驱动结构体字段明确为：

- `DeadTimeCH012N/P`：CH0、CH1、CH2 共用的一组 N/P 死区计数。
- `DeadTimeCH345N/P`：CH3、CH4、CH5 共用的一组 N/P 死区计数。

HAL 实现也只写入 DTH00/DTH01 和 DTH10/DTH11 四个寄存器。仓库转换版手册的部分文字描述“每对互补 IO 独立死区”与当前 SDK 字段/寄存器分组存在表述不一致。A2 只测 CH0，不受此差异影响；在扩展三相前，应再用当前官方手册 v1.69 的寄存器表与实际芯片读回确认，不应假设 CH0/1/2 可分别设置不同死区。

## 6. A2 使用的单组配置

- 基准：MCLK 48MHz RC，MCPWM 分频 1。
- `TH0 = 2400`，沿用 A1 的约 10.086kHz 载波。
- `TH00 = -600`、`TH01 = +600`，沿用 A1 的比较窗口。
- `MCPWM_WorkModeCH0 = MCPWM0_CENTRAL_PWM_MODE`。
- `CH0_PS = 0`、`CH0_NS = 0`，P/N 均由硬件控制。
- `IO0_PPE = DISABLE`。
- `DeadTimeCH012N = 48`、`DeadTimeCH012P = 48`。
- P1.4 = CH0P，P1.5 = CH0N，二者均选择 AF3_MCPWM。
- CH0P/CH0N 关闭默认值均为低；其他四路保持 GPIO 低。
- START1/STOP1 控制输出使能；STOP 优先。

## 7. 验证边界

A2 仅验证 MCU 的逻辑 PWM 输出，不给功率母线供电、不连接电机、不以此证明栅极驱动或功率器件安全。示波器应同时观察 MCU 侧 P1.4、P1.5，测量两个换向方向的非重叠时间，并确认停止/复位时两路回到低电平。
