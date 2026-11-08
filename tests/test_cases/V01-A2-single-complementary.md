# V0.1-A2 — CH0P/CH0N 单组互补 PWM 验证

- Status: **COMPLEMENTARY PWM / FREQUENCY / DEADTIME OBSERVED; EDGE-RINGING ISSUE OPEN; FULL A2 ACCEPTANCE PENDING**
- Scope: MCU logic-level validation of one complementary PWM pair only.
- Target: LKS32MC071CBT8, LKS_EVB_MCU071CBT8 + LKS_EVB_MVPOWPRE.
- Safety: inverter DC bus physically disconnected; FN550-90K and motor phases disconnected.

## A2-0 资料核查结论

已交叉检查：
- 仓库内 LKS32MC07x MCPWM 用户手册转换版：`firmware/projects/keil/lks32mc071/lks32mc07x_periph_driver/user_manual/13_MCPWM.md`。
- 官方标准外设驱动库 V3.16：`MCPWM_InitTypeDef`、`MCPWM_StructInit()`、`MCPWM_Init()`、`PWMOutputs()`。
- 同一 SDK 的 HAL MCPWM 实现：用于交叉核对死区计数换算；注意 HAL MCPWM 源文件未加入当前 Keil 主目标。
- 官方网站目前列出 LKS32MC07x 用户手册 V1.69（2026-07-11）、标准驱动库 V3.16（2026-08-07）和外设 Demo V1.9（2026-08-30）。独立 Demo 压缩包下载在本次核对中超时；本次具体配置以仓库所用 V3.16 标准驱动源码及其中的 MCPWM_Init 示例为准。

官方资源入口：
- [LKS32MC07x 用户手册列表（官网，当前列出 V1.69）](https://www.lksmcu.com/index.php/UserManual/)
- [LKS32MC07x 外设驱动/外设 Demo 列表（官网，当前列出驱动 V3.16、Demo V1.9）](https://www.lksmcu.com/index.php/Reference_2/)
- [LKS32MC07x Peripheral Demo V1.9 压缩包](https://sjk.lksmcu.com/static/upload/file/20230113/lks32mc07x_demo_prj_v1.9.zip)

官网 PDF/ZIP 在本次远程抓取中出现超时；上述版本号来自官网资源列表，具体寄存器配置本轮以仓库当前 V3.16 标准外设驱动源码及其 MCPWM_Init 示例交叉核对，未声称已逐文件审阅独立 Demo 压缩包。

结论：
1. `MCPWM0_CENTRAL_PWM_MODE=0` 是中心对齐设置，不是推挽开关。
2. `IO0_PPE` 才是 P/N 推挽交替使能；A2 必须保持 DISABLE。
3. CH0P/CH0N 互补模式下，`CH0_PS=0`、`CH0_NS=0` 才让两路都由 MCPWM 硬件控制。A1 的 `CH0_NS=1` 是将 N 路强制为软件电平，A2 必须取消。
4. 中心对齐互补输出在 `TH00` 处先关闭 N，延迟 DTH0 后打开 P；在 `TH01` 处先关闭 P，延迟 DTH1 后打开 N。
5. 当前标准驱动结构体将 `DeadTimeCH012N/P` 映射到 CH0/1/2 共用的一组 N/P 死区计数；CH3/4/5 使用另一组。仓库转换版手册部分文字描述与结构体字段分组不完全一致。A2 仅测 CH0；扩展三相前需要再核对官方当前手册和芯片读回。
6. HAL 的纳秒死区换算使用 `HAL_CLK_GetMcuFreq()/MCPWM时钟分频`。A1 实测载波 10.086kHz、TH0=2400，反推 MCPWM 时钟约 48.42MHz。A2 使用 48 个计数，估算死区约 0.99µs。该值仅用于逻辑级示波器验证，不是最终功率级死区。

## 实测补充：边沿振铃（2026-11-08）

A2 互补输出波形已观察到约 10.1kHz 两路互补 PWM，正脉宽约 24% / 74%，死区约 1µs。边沿附近存在衰减振铃，已作为独立硬件遗留问题记录：
- MCU P1.4 对 MCU 本地 GND，×10 探头、短弹簧地：负向最低值约 -0.8V。
- 预驱输入端对预驱本地 GND：负向最低值约 -1.2V。
- 正向过冲可见，但本次未记录可靠峰值。
- 缩短测量回路/信号连接后，振铃幅度有所下降；根因尚未完全分离。
- 当前底板不便串入阻尼电阻，因此不在现板强行改线。下一版自绘底板考虑预留 MCU 端串联阻尼电阻、预驱输入端可选 RC 焊盘，以及邻近信号/GND 测试点。

详细现象、风险、候选改进及关闭条件见 [HW-001：MCPWM 输出边沿振铃与负向过冲](../../hardware/issues/HW-001-mcpwm-edge-ringing.md)。**该问题仍为 OPEN / DEFERRED，不得视为已经解决。**

## 硬件引脚

| 信号 | MCU 引脚 | 连接器/网络 | 复用 |
|---|---|---|---|
| CH0P | P1.4 | UP | AF3_MCPWM |
| CH0N | P1.5 | UN | AF3_MCPWM |
| CH1P/CH1N | P1.6/P1.7 | VP/VN | 测试期间 GPIO 低 |
| CH2P/CH2N | P1.8/P1.9 | WP/WN | 测试期间 GPIO 低 |
| START1 | P2.11 | 按键 | 低有效 |
| STOP1 | P2.12 | 按键 | 低有效 |

## A2 初始参数

| 配置 | 初值 | 说明 |
|---|---:|---|
| MCLK | 48MHz RC | 与 A1 保持一致 |
| MCPWM CLK_DIV | 0（÷1） | 不分频 |
| TH0 | 2400 | 约 10.086kHz，按 A1 实测 |
| TH00 | -600 | P/N 换向比较阈值 0 |
| TH01 | +600 | P/N 换向比较阈值 1 |
| CH0 工作模式 | CENTER | 中心对齐 |
| CH0_PS / CH0_NS | 0 / 0 | P/N 均为硬件输出 |
| IO0_PPE | DISABLE | 禁止推挽周期交替 |
| DeadTimeCH012N/P | 48 / 48 counts | 约 0.99µs，两方向相同 |
| CH0P/CH0N default | LOW / LOW | 输出关闭时安全低 |
| 其他四路 | GPIO low | 不切换 AF3 |

按该阈值和初始死区估算，P/N 两路频率均约 10.086kHz；两次换向均应出现约 0.99µs 的 P/N 同时低电平间隔。P 路高电平时间预计约 23.8µs（约 24%），N 路高电平时间约 73.4µs（约 74%）；最终以示波器实测为准。

## Keil 启用方式

1. 拉取最新仓库。
2. 主目标选择 `Linko 07x Project`。
3. 在 Options for Target → C/C++ → Define 中移除 `BENCH_TEST_MCPWM_SINGLE`，添加 `BENCH_TEST_MCPWM_COMPLEMENTARY`。
4. Rebuild All，确认 0 Error 后再下载。
5. 主功率母线断电，电机断开。不要因为 A2 逻辑波形通过就给功率级上电。

## 示波器接法

- CH1：MCU P1.4 / CH0P。
- CH2：MCU P1.5 / CH0N。
- 两个探头地夹均接已确认的 MCU 逻辑 GND。
- 不要把探头地夹接到开关节点、BUS− 或不确定的功率参考点。
- 使用两个通道同时观察完整周期，并放大换向边沿测量两处死区。

## 按键测试

- 上电默认输出关闭，P1.4/P1.5 均低。
- START1 短按后，P1.4/P1.5 同时输出互补 PWM。
- 松开 START1 后保持输出。
- STOP1 按下后，两路关闭并恢复 GPIO 低。
- 同时按下 START1/STOP1 时 STOP 优先。
- 复位和上电期间两路不得出现异常高脉冲。

## 验收清单

- [ ] Rebuild All 0 Error，下载的 HEX 对应当前 A2 构建。
- [ ] P1.4、P1.5 各自频率约 10.086kHz，周期一致。
- [ ] 两路逻辑电平极性正确，正常工作时不同时为高。
- [ ] N 关闭到 P 打开之间的死区约 0.99µs。
- [ ] P 关闭到 N 打开之间的死区约 0.99µs。
- [ ] START1 启动、STOP1 关闭、双键 STOP 优先均通过。
- [ ] STOP/复位/上电后 P1.4、P1.5 均保持低电平。
- [ ] P1.6～P1.9 始终保持低电平。
- [ ] MCU/预驱输入端边沿瞬态的绝对最大额定值核对与风险关闭（关联 HW-001，当前未完成）。
- [ ] 已归档示波器波形和实际死区测量值。

A2 通过仅代表 MCU 逻辑互补 PWM 与死区配置验证完成，不代表预驱、功率器件或电机功率级通过验证。
