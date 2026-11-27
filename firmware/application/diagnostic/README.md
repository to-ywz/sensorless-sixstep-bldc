# DEBUG-01 UART 与 VOFA+ 接口

## 构建与接入

在 Keil 主 Target `Linko 07x Project` 中，用 `BENCH_TEST_CURRENT_SAMPLE,BENCH_DEBUG_UART` 替换默认 Bench 宏来选择 M3 UART 版本。只用 `BENCH_TEST_CURRENT_SAMPLE` 则构建 M3 无 UART 版本。仓库默认宏仍是 `BENCH_TEST_CMP_FAIL,M2_P1_DYNAMIC`。

串口配置为 115200 bps、8N1、无流控。上电默认输出 JustFloat，20 Hz。VOFA+ 中选择串口及 JustFloat 数据引擎。发送指令时使用 ASCII，关闭十六进制发送，并追加 CR 或 LF。协议依据 [VOFA+ JustFloat](https://www.vofa.plus/docs/learning/dataengines/justfloat/)。

## 3,000,000 bps 实验配置

在 M3 UART 配置中再加 `BENCH_DEBUG_UART_3M_EXPERIMENTAL`，并在 VOFA+ 手动选 3000000、8N1。去掉该宏即恢复 115200。两种配置沿用 20 Hz 默认遥测、1–100 Hz RATE 范围和 JustFloat-M3/1。

当模块时钟为 48 MHz、共享预分频为 0 时，UART0 的 DIVH=0、DIVL=15，名义速率为 3,000,000 bps。凌鸥 [LKS32MC07x 数据手册 §23](https://www.lksmcu.com/static/upload/file/20230113/LKS32MC07x_Datasheet_EN_v1.21.pdf) 所列支持范围为 300–115200 bps。3 Mbps 已通过 PWM 停止条件下的双向通信实测。时钟误差容限及对 PWM 运行时 M3 采样的影响仍待测。CH343 标称峰值可覆盖 3 Mbps，实际链路取决于接线和电平。见 [CH343 芯片资料](https://www.wch-ic.com/downloads/CH343DS1_PDF.html)。

3 Mbps、8N1 每字节在线时间约 3.33 μs，相当于 48 MHz 下 160 个周期。沿用的逐字节中断和单字节硬件缓冲尚未验证持续满速收发。先在 PWM 停止、母线断开、电机未接时测位宽、20 Hz 序号连续性、逐条文本命令和 `STATS`。PWM 运行时的实验需另外对比 115200 配置的漏采、时序故障与 STOP 响应。此配置不提高遥测频率或声明连续吞吐能力。

## UART 资源审计

| 资源 | 核查结论 |
|---|---|
| MCU | LKS32MC071CBT8，UART0/1 均支持全双工、8/9 数据位、1/2 停止位、可选校验。UART0 采用 AF4。 |
| TXD0 | 核心板 MCU P1.0／芯片脚 18，经板间 P1-10 到功率底板，经过 R12 10 Ω 至 P4-2。 |
| RXD0 | 核心板 MCU P0.15／芯片脚 17，经两板 P1-25 到功率底板，经过 R9 10 Ω 至 P4-3。 |
| UART1 | TXD1 为 P3.9／芯片脚 26，经 R21 10 Ω 到两板 P1-8。RXD1 为 P2.8／芯片脚 25，经 R22 10 Ω 到两板 P1-23。UART1 留作后续资源。 |
| 时钟 | 48 MHz 内部 RC。UART0 局部分频写入 DIVH/DIVL，保留 UART 共用的 SYS_CLK_DIV2。共用预分频为 0 时本地除数为 417，名义 115107.9 bps，误差约 -0.08%，未计 RC 公差。 |
| IRQ | UART0 中断取 NVIC 最低优先级。每次处理最多一个接收字节和一个发送完成。M3 的 MCPWM/ADC/STOP 为主循环轮询，NVIC 优先级无法阻止 UART 抢占前台。 |
| 资源冲突 | UART0 两脚与现有 MCPWM P1.4–P1.9、STOP P2.12／两板 P1-12、START P2.11／两板 P1-26、ADC/OPA、CMP1/FAIL1、SWCLK/SWDIO 和 P0.6 心跳分离。诊断复用 M3 已有 SysTick 读数，未占用 Timer 或更改 ADC/MCPWM 调度。 |
| 历史调试 | 基线由计划记录及源码确认已撤回 scope toggle。曾观测 P4-2 随取码翻转，仅作为该网络的历史证据。 |
| 电平与供电 | P4-1 GND，P4 只有地与两条串口信号，R9/R12 为串联电阻，板图未示电平转换，核心板 MCU 的 AVDD 接 +5 V。试验前核实双方 VIH/VIL、输入耐压、空闲电平和供电状态。适配器 TX 接 P4-3，适配器 RX 接 P4-2，共地，适配器 VCC 悬空。实际逻辑电平裕量和板实物修订仍为 NOT VERIFIED。 |

板图来源为 LKS_EVB_MCU071CBT8_V1.0 和 LKS_EVB_MVPOWPRE_V5.0，第 1 页。复用由仓库 SDK 和 LKS32MC07x UART Demo 的 GPIO 配置交叉核对。器件规格见 [厂商资料](https://www.lksmcu.com/index.php/LKS07Series/)。

## JustFloat-M3/1

每帧固定为 16 个 IEEE 754 float32 小端字，随后是 `00 00 80 7F`。总长 68 字节。

| CH | 字段 | 单位及语义 |
|---:|---|---|
| 0–2 | raw_u, raw_v, raw_w | ADC 有符号原码，code |
| 3–5 | current_u, current_v, current_w | 经标定电流，A |
| 6 | sector | 现有六步扇区 0–5 |
| 7 | pwm_cycle | 样本所属 PWM 周期，低 24 位 |
| 8 | running | 快照时 PWM 是否运行 |
| 9 | frame_current | 快照时采样帧尚未作废，允许同一采样连续多次发布 |
| 10 | raw_mask | U/V/W 原码接收位 0/1/2 |
| 11 | valid_mask | U/V/W 当前已标定有效电流位 0/1/2 |
| 12–14 | status_u, status_v, status_w | 现有状态枚举 0 OK、1 未选、2 扇区错误、3 窗外、4 缺失、5 过期、6 饱和、7 未标定 |
| 15 | seq | 每次启动一次遥测编码递增，发送低 24 位 |

无原码或无有效电流时填 0，分别以对应掩码解释。非有限浮点强制填 0。启动后首帧前各相状态为缺失。STOP 后允许保留历史原码与状态，CH9 为 0 且 CH11 清零。软件静态采样的 PWM 周期为 0，可用 CH8、CH9 联合判断。周期和序号在模 2²⁴ 下精确表示，转折点按回绕解读。周期低 24 位回绕也能产生零。遥测序号描述发送尝试，丢帧或中途作废会出现间隔。需要辨认采样次数时，在文本模式用 STATUS 的 32 位采样计数。

20/50/100 Hz 对应 1360/3400/6800 B/s，约为线路理论容量的 11.8%/29.5%/59.0%。50 Hz、100 Hz 的实际可用性待实测。

## 文本命令

指令大写，最长 31 字节，超长或非打印字符使整行作废。收到 RX 溢出或错误后，发送一个空行重新定界，再发送完整命令。切换由发送完成中断确认，已入队的数据按原模式发完。

| 指令 | 行为 |
|---|---|
| MODE TEXT | 等二进制队列及线上最后一字节发送完毕，返回 `OK TEXT` |
| MODE JUSTFLOAT | 文本发完后恢复二进制，按当前 STREAM 配置发帧 |
| HELP / VERSION | 文本模式查询命令及协议版本 |
| STATUS | 文本模式读 running、frame_current、采样次数和时序故障 |
| SAMPLE | 文本模式读 raw_mask、U/V/W 原码，原码按 32 位补码打印 |
| STATS | 文本模式读 TX 丢弃记录、RX 软件溢出字节、RX 硬件错误事件和拒绝命令数 |
| STREAM 0 / STREAM 1 | 关闭或开启二进制遥测 |
| RATE n | 1–100 Hz，默认 20 Hz |
| LOG n | 0–3，默认 2，控制文本日志 API 的等级 |

文本数值每项为八位十六进制。运行状态变化可在文本模式轮询输出 EVENT，快速往返状态可能合并。JustFloat 模式中的文本响应和日志保持静默。所有命令只作用于诊断，原有调试器 Bench 命令与物理 STOP 独立运行。

## 实时性与验证

前台是 M3 帧唯一写入者，诊断同前台按值读取帧和状态。UART ISR 只接触传输缓冲。TX 先复制再在局部屏蔽 UART IRQ 时发布写指针。RX 出队、错误恢复和统计快照也仅屏蔽 UART IRQ，临界段限于少量索引、标志和一次发送启动。TX 256 字节、RX 64 字节，各留 1 字节区分空满。空间不足时整条记录丢弃。

运行时先轮询 STOP 和采样，再在已完成当前样本、CNT0 处于 0–1000 且无新边界的时段中执行一个诊断步骤。编码逐通道执行，Cortex-M0 软件 64 位转 float 只用于对应有效且已标定的相。文本回复使用有界十六进制格式化。UART 满速连续 RX 与发送完成可合计约 23040 次事件/秒，重叠事件可在同一 ISR 处理。软浮点、命令回复、队列复制和 IRQ 叠加的最坏执行时间尚未测定，1400 tick 的距离为调度预留。任何新增归属错误、漏采或物理 STOP 延迟都需要回到诊断调度及 IRQ 负载解决。

从仓库根运行以下独立验证。默认全量命令继续执行保护静态门禁。

```sh
python tests/unit/run_lks560_tests.py --diagnostic-only
python tests/unit/run_lks560_tests.py --current-only
python tests/unit/run_lks560_tests.py --outputs-only
python tests/unit/run_lks560_tests.py
```

## 实板通信结果（2026-11-27）

使用 CH343（Windows COM8），115200 和 3000000 均为 8N1、无流控。以下是串口功能证据。两次速率复验期间均未通过 UART 发出电机操作。

| 配置 | 证据 | 判定 |
|---|---|---|
| 115200（默认） | 3 秒采集窗口内解析到 59 个完整帧，序号连续。两轮 `MODE TEXT`、`VERSION`、`STATUS`、`STATS` 与 `MODE JUSTFLOAT` 往返通过。TX 丢弃与 RX 溢出均为 0，累计 RX 错误 21、拒绝命令 2，在两轮确认中均未增加。用户确认 VOFA+ 波形显示正常。 | 双向通信与 VOFA+ 显示通过 |
| 3000000（实验配置） | `STATUS` 确认 PWM 停止。15 秒采集窗口内校验 301 个完整 JustFloat 帧，序号 1723 至 2023 连续，无坏帧。文本命令、回显及恢复遥测通过，`STATS` 四项计数均为 0。 | PWM 停止时的 20 Hz 遥测与双向通信通过 |

3 Mbps 完整实测记录的文本响应为 `V0.2 DEBUG-01 JustFloat-M3/1`，恢复二进制后 3 秒校验 59 帧。6 Mbps 仅有分频计算及适配器标称规格，尚未配置或测试。其它常用速率留作后续配置候选。全量主机回归仍受基线 `config_valid` 断言阻塞。

## 后续实板验收（需单独授权）

1. 母线断开且电机未连接。先核对板卡修订、P4 通断、MCU 与适配器输入电平，再测 TX 位宽和 RX 收发。验证文本模式退出、返回 JustFloat、帧长及 16 通道次序。
2. 停止 PWM 时使用既有 M3 STATIC 命令。复查三相静态注入、回零、原码掩码、无标定状态及 CH7=0。以同工况对比无 UART 版。
3. 以既有无母线固定扇区工况，对比无 UART 与 20 Hz 版的采样归属、漏采、时序故障和运行负载。对照记录正常接收、断开、连续命令和拥塞时的物理与软件 STOP 及历史帧标志。
4. 在 20 Hz 无新增故障且 STOP 对照通过后，分别验证 50 Hz、100 Hz，记录实际发送率、丢弃、IRQ 时间和采样时序。任何新增故障均暂停升频并回退诊断配置。
