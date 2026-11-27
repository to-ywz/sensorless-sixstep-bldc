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
| 8 | running | MCU 取得遥测快照时 PWM 是否运行 |
| 9 | frame_current | MCU 取得遥测快照时，所引用的采样帧尚未作废；同一采样可多次发布 |
| 10 | raw_mask | U/V/W 原码接收位 0/1/2 |
| 11 | valid_mask | U/V/W 当前已标定有效电流位 0/1/2 |
| 12–14 | status_u, status_v, status_w | 现有状态枚举 0 OK、1 未选、2 扇区错误、3 窗外、4 缺失、5 过期、6 饱和、7 未标定 |
| 15 | seq | 每次启动一次遥测编码递增，发送低 24 位 |

无原码或无有效电流时填 0，分别以对应掩码解释。非有限浮点强制填 0。启动后首帧前各相状态为缺失。STOP 后允许保留历史原码与状态，CH9 为 0 且 CH11 清零。软件静态采样的 PWM 周期为 0，可用 CH8、CH9 联合判断。周期和序号在模 2²⁴ 下精确表示，转折点按回绕解读。周期低 24 位回绕也能产生零。遥测序号描述发送尝试，丢帧或中途作废会出现间隔。需要辨认采样次数时，在文本模式用 STATUS 的 32 位采样计数。

20/50/100 Hz 对应 1360/3400/6800 B/s，约为线路理论容量的 11.8%/29.5%/59.0%。50 Hz、100 Hz 的实际可用性待实测。

字段的时间语义分两部分。ADC 原码、扇区、PWM 周期和采样原因属于 ADC 样本。CH8 和 CH9 属于 MCU 取得该遥测快照的时刻。主机收到帧时，PWM 可能已经停止或切换扇区。STOP、换相重启及静态采样替换会推进内部失效代次。若代次在快照取得后、进入 TX 队列前改变，就丢弃待编码帧，即便 CH8 和 CH9 后来恢复原值。已经整帧入队的数据作为历史记录按原顺序发送，原字段保持采集值。接收端不得使用 CH8/CH9 判断 STOP 是否已经生效。失效代次只用于内部检查，线上仍为 16 通道。

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

运行时先轮询 STOP 和采样，再在已完成当前样本、CNT0 处于 0–1000 且无新边界的时段中执行一个诊断步骤。编码逐通道执行，Cortex-M0 软件 64 位转 float 只用于对应有效且已标定的相。文本回复使用有界十六进制格式化。115200 下满速连续 RX 与发送完成可合计约 23040 次事件/秒，重叠事件可在同一 ISR 处理。软浮点、命令回复、队列复制和 IRQ 叠加的最坏执行时间尚未测定，1400 tick 的距离为调度预留。任何新增归属错误、漏采或物理 STOP 延迟都需要回到诊断调度及 IRQ 负载解决。

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

表中为最终修订前的实板记录。最终修订后，用户重新烧录 3 Mbps 实验版，最新 Keil MAP 含 `frame_epoch`。COM8 以 3000000、8N1 在 PWM 停止时复验，5 秒校验 101 个完整帧，序号 2014–2114 连续，无坏帧。切换文本模式收到 `OK TEXT`，`VERSION`、`STATUS`、`STATS` 回复正常，TX 丢弃、RX 溢出、RX 错误和拒绝命令均为 0。切回 JustFloat 后再校验 60 个完整帧。该结果覆盖修订版通信，跨 STOP 的快照行为由专项 Mock 验证。运行实时性对照待执行。

最终修订的 DEBUG-01、M3、A0–A4 专项 Mock 均退出 0。四套 Keil 配置（默认主 Target、M3 无 UART、M3 + 115200、M3 + 3 Mbps）离线重建均为 0 错误、0 警告。全量主机回归仍受基线 `config_valid` 断言阻塞。

## 最小运行实时性对照

### 本轮实测（2026-11-27，收尾未通过）

固件基线为 `3e13d73bdaa3446400e826fddb6f0af7d48efed8`。已确认隔离条件并用 Keil 烧录校验 A、B 固件。两组均为固定扇区 0、250‰ 占空比，各采集三次约 10 秒的稳定窗口。

| 组别 | 软件 PWM 周期增量 | ADC 帧增量 | timing_faults 增量 |
|---|---|---|---|
| A，无 UART | 100898 / 100899 / 100906 | 100899 / 100899 / 100907 | 0 / 0 / 0 |
| B，115200 / 20 Hz | 100904 / 100897 / 100890 | 100904 / 100897 / 100890 | 0 / 0 / 0 |

PWM 周期来自 `g_m3_pwm_debug`。周期与帧计数分开读取，差 1 的窗口不据此判定漏采。SWD 抽样的扇区均为 0、原码掩码均为 2，未见新增时序故障。缺少同步 PWM 边沿与 ADC 采样记录，完整漏采和归属对照仍未关闭。

- B 组 PWM 运行时，COM8 抓取 12 秒收到 0 字节。SWD 读到 ADC 计数继续增长，遥测序号和 TX 队列保持原值。软件 STOP 后，3 秒重新解析到 60 个完整帧，序号连续。试验性窗口调整未恢复运行遥测，已撤回。
- 两组软件 STOP 均观察到 `g_m3_running=0`、`g_m3_frame_current=0`。B 组的按键观测窗口结束时已停机，A 组相同窗口结束时仍在运行，随后由软件 STOP 停机。没有可复核的同步波形，物理 STOP 延迟、测量分辨率及 B 组 STOP 与 UART 发送的重叠均为 **NOT VERIFIED**。软件 STOP 的延迟也为 **NOT VERIFIED**。
- 本轮未确认 B 组运行期间的 VOFA+ 刷新。此前停机通信和显示证据继续保留。测试结束板上为 A 组固件，读回确认主输出使能 MOE 为 0。

### 复测方案（实板需单独授权）

母线断开、电机未接。使用同一套板、电源、探头及测量窗口，沿用 M3 固定扇区 0 和 250‰ 占空比。先构建 A（`BENCH_TEST_CURRENT_SAMPLE`，无 UART），再构建 B（再加 `BENCH_DEBUG_UART`，115200、20 Hz JustFloat）。每种配置做三次同长度的稳定窗口和 STOP 对照。记录固件标识、观测窗口、触发条件和测量分辨率。

| 对照项 | 留证方法及判定 |
|---|---|
| 采样计数、时序故障与漏采 | 记录 `g_m3_frame_count`、`g_m3_timing_faults`、`g_m3_pwm_debug.finish_count` 的窗口差值，以示波器 PWM 周期数对照。发布计数含失败样本，结合 `reason` 与原码掩码判定漏采和超时。A、B 使用同样的读取方式，稳定窗口内不设暂停断点。B 的漏采和时序异常不得高于 A。 |
| 样本归属 | 对照 `g_m3_frame.pwm_cycle`、`sector`、`raw_mask` 和 `g_m3_pwm_debug` 的周期及完成原因，检查固定扇区和一次已知换相重启边界。B 不得出现新增跨周期、跨扇区误归属。重复遥测按同一 ADC 采样解读。 |
| 物理 STOP | A、B 都从相同运行工况触发按键。B 要在 UART 正在发帧时覆盖至少一次 STOP。同步捕获 STOP 输入下降沿、UART TX 和 MCU PWM 输出进入安全态的时间，记录每次延迟及最坏值。 |
| 软件 STOP | 使用既有调试器 STOP 命令，A、B 采取同一写入和恢复运行方法。记录首次可观测 STOP 请求到安全态的时间和测量分辨率。若只能观察停机结果，应注明延迟未验证。 |
| 停机结果 | 两类 STOP 均须进入既有安全态，`g_m3_running` 与 `g_m3_frame_current` 清零。对照 B 的延迟是否超出 A 的实测波动及测量分辨率。UART 线上可先收到入队历史帧，随后才收到停机后的快照。 |

如出现新增漏采、错误归属或 STOP 延迟退化，保留对照证据并复核调度。100 Hz 遥测与 3 Mbps 下 PWM 运行归入后续性能扩展，基础通信功能关闭不以这些项目为前置条件。
