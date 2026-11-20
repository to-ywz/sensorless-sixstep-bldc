# M2 Ready-to-Test 实际证据表

- 日期：2026-11-18；期限检查点：2026-11-20结束前。
- **当前：NOT READY。** 用户整体授权已获得；技术就绪、用户现场参与和具体操作条件仍须满足。
- 本表不得因脚本自检或某个资格标志为1而自动转READY。没有实际证据的行保持缺项。

| 检查项 | 当前证据 | 状态/缺项 |
|---|---|---|
| 基线与既有文件保护 | 正式HEAD=f739977；原有未提交文件已识别；研究在基线导出副本进行 | 基线确认；未改变原文件 |
| A最小配置冻结 | 配置检查点已建立 | 未完成，输入/窗口/激励待关闭 |
| B待测固件 | 只有旧版部分实现；旧WP2结果复用但不代替新固件 | NOT READY，真实CMP/RAW、FAIL、有限PWM、CLEAR、观测段及版本指纹未完成 |
| 外部输入配置 | 图纸OC接P2.1且有R24/R25/C9负载 | NOT READY，实物负载、幅度/共模、极性、阈值与激励方案缺证据 |
| 独立核心板供电 | 用户现状为32V辅助转换15V/5V；该15V按图也供LKS560 | NOT READY，接口/分板/供电冲突及逻辑轨实际条件待核对 |
| 六路物理隔离 | 尚无断电通断或等效物理断开证据 | NOT READY，仅断母线不合格 |
| 母线/电机排除 | 用户说明母线断开 | 文字证据；实际P3、接线及电机状态待核对 |
| 接线与地参考 | 核心板/功率板图纸已取得 | NOT READY，最终接线图和源/探头接地未冻结 |
| 仪器 | 用户DHO914，无独立发生器 | NOT READY，探头及合格外部激励方案待确认 |
| 验收判据 | H1/H2/S1/R1与P低/N高保持R1定义 | NOT READY，时间上限、最小故障脉宽、重复条件与观测段时长待冻结 |
| 现场参与与时间 | 工作日≤2h、周六全天、周日≤3h | 实验预约待RTT完成后安排；不能假定本轮可操作 |

## READY时必须补入的证据

每项记录实际来源、日期、操作者/提供者、配置/固件指纹和结果。硬件接线与隔离证据由用户参与取得；研发侧负责明确步骤和判据，用户不必一次完成全部检查。

RTT负责人核对所有必需行后再报告READY；届时明确适用配置和实验范围。任何接线、供电、阈值或待测固件变更都需要复核受影响行，不能沿用不适用证据。

## 未就绪的排期处理

11月20日仍NOT READY，则11月21日不进入C，改为关闭缺项；不减少安全检查或把内部DAC测试替代外部注入。11月28日只作为候选补测窗口，不保证解决全部硬件问题。

当前不开展C/D，不给出其PASS结论，也不将M2授权扩大到驱动/功率级/电机实验。

## 2026-11-19 P0 更新

用户确认独立核心板上 P0.0 经外部连线接 OC 时，静态高低档分别得到 RAW=1/0。外部输入静态翻转已有证据；OC 带载电压、脉冲条件和受保护 PWM 等缺项保留，RTT 仍为 NOT READY。

## 2026-11-19 FAIL1 路由预检

隔离工程主 Target 完整编译链接，0 错误、0 警告。HEX SHA256：0fe3923a002f8386bc3714b28a4660e0dc875166d4de7631e06787788c376200。保持独立核心板、AF0 安全输出、CNT0/MOE 关闭。预检在 RAW=0 且 CMP1 窗口位全零时清除历史 FAIL1_IF 并单次使能高有效 FAIL1。复用 P0.0 到 OC，先 stimulus=0，再写 probe_command=1，确认 probe_state=1、EIF bit5=0、MOE=0 后令 stimulus=1，再回 0。记录 RAW、EIF bit5、MOE；首次置位仅说明静态事件到达。若 RAW 翻转但 EIF 不置位，继续查窗口/滤波等语义，不开 PWM。预检实测待执行，完整 P1/H1/H2 仍未验证，RTT 仍 NOT READY。

## 2026-11-19 FAIL1 路由预检实测

- `hardware/measurements/raw/2026-11-19_M2/2026-11-19_M2_01_FAIL1_RAW-high.png` SHA256 `9f18890e6ebdabc51e7ab7febe7e53fb55872652bef4e55ef098e2bf832863d5`
- `hardware/measurements/raw/2026-11-19_M2/2026-11-19_M2_02_FAIL1_RAW-withdrawn.png` SHA256 `5d2727f77c2e45e737feb75de60e072d65ee4102b46f599a87f15300b9bc96fc`

高档 RAW=1、EIF=0x20、MOE=0。撤除后 RAW=0、EIF=0x20、MOE=0。本配置下故障抵达 FAIL1，事件标志在撤除后保留。截图未覆盖预检武装前 EIF 基线或 probe_state。MOE 从始至终关闭，未验证硬件切断运行中的 PWM 或独立保持输出关闭。H1/H2 仍待同步波形实测。


## P1 限时 PWM 与同步观测准备

- 路由预检已经有高档 RAW=1/EIF=0x20、撤除 RAW=0/EIF=0x20 的证据。进入动态试验时由固件在禁止输出、RAW=0 状态读取并保存清标志前后值，确认本次事件从 EIF bit5=0 开始。无需用户重做整组 Watch 测试。
- 台架沿用独立核心板，P0.0 经外部连线到 OC。两个探头即可：CH1=OC，CH2=UP（P1.4，核心板 P1-22），地夹接核心板同一 GND。以 OC 上升沿单次触发，带预触发，直接记录带载 OC 电平和运行中的 UP。三相覆盖另在 P2 逐一复测。
- 候选调度用于先证明功能：约 10 kHz / 25% UP，输出约 2 ms 后施加约 0.5 ms 单次故障，撤故障后再观测约 2 ms，最后执行软件 STOP。以上是待实现的台架调度初值，非硬件延迟或最小可检故障脉宽的验收值。
- 输出由 Manager 经平台授权，禁止复用旧 A4/A5 直接置 MOE 路径。P1 专用实验就绪和正式硬件资格分开建模。保留 FAIL1_SEL=1、高有效、CMP1 窗口位 0、FLT=0、AEC=0 的预检配置作为候选，动态有效性和时钟待证实。
- 现有诊断 poll 每轮调用 Protection_Process，且 inspect 会请求清 MOE。这条路径不能进入 P1 观测段，否则软件可能掩盖硬件动作。有限观测段只允许切换 P0.0 激励、读取状态和计时，禁止写 MOE、STOP 或改 PWM 引脚复用。到期或显式中止后必须回到关断并交 Manager 锁存，记录原因及是否提前中止。默认无激励、无输出，故障后不自动重开。
- 首轮同时看两个现象：H1 看 OC 有效后 UP 截断且不再出现后续脉冲；若注入恰逢 PWM 低电平，另记录不同相位捕获，以取得截断证据。H2 看 OC 已回低、软件仍未执行 STOP 的窗口内 UP 保持低电平。保存实际软件 STOP 时点或可对齐的计时证据。若软件提前介入、未捕获有效相位或没有前置 PWM，相关截图不作完整 H1/H2 证据。
- RTT 仍 NOT READY。待确认带载 OC 电平和边沿、冻结本轮观测与延迟判据、完成 Manager/Platform/Bench 实现并构建可识别映像。本节作为实现与采集准备，无新 PWM 固件或实测结论。


## P1 动态固件实现与构建

隔离主 Target 使用 BENCH_TEST_CMP_FAIL,M2_P1_DYNAMIC。上电默认无输出，初始化核对转换前 EIF、RAW=0、FAIL1 位与关闭态。P1 快照 config_valid 表示本次实验配置，hardware_qualified 仍为 0；清故障恢复接口仍阻塞。P1 请求经 Manager，单次启动，仅 UP 切至 MCPWM，其他五路保持 GPIO 安全态。一次运行后终止，重复需先复位。P0.0 激励约在启动后 2 ms 拉高，约 0.5 ms 后拉低，再观察约 2 ms 后请求 STOP。活跃阶段不调用常规 Protection_Process，提前中止单列。计时由 SysTick 软件轮询，实际引脚边沿以示波器为准，勿在活跃期间暂停调试。

状态 1 待命、2 活跃、3 正常走完调度、4 拒绝或提前中止、0 初始化未就绪。command=1 运行一次，command=2 请求中止。状态 3 不代表硬件通过。保存 before_fault/before_release/before_stop/after_stop 四组快照以及 inject/release/stop tick。若 before_release 和 before_stop 的 RAW 未分别为 1/0，或 EIF 未置位、MOE 未降为 0，保留失败证据。H1/H2 的判断须同时核对 OC 和 UP，H1 的正式延迟上限及最小脉宽尚待冻结。此版仅供已隔离核心板的 P1 功能定位，未关闭 RTT、H1/H2 或 P2/P3。

主 Target 完整编译和链接，0 错误、0 警告。日志 P1-dynamic-build.log。
HEX SHA256：58f000994a0e3f854585963c15f24207ca685fe7caacf6b65d7e3399beb9a3b6。

## P1 首轮采集

Watch 状态 3、command=0。动态分支单独初始化，旧 P0 ready=0 不判故障。CH1 自触发捕获约 99.128 us 周期的 PWM，CH2 未见清晰 OC 方波。接线/通道身份待确认，不能判 H1/H2。原图 03/04 已归档。下一步读取 before_release 和 before_stop 快照区分比较器输入与采集问题。

## P1 首轮快照复核

撤激励前及 STOP 前均为 RAW=0、EIF=0、MOE=1、FAIL012=0x460。FAIL1 已选源并使能，但本次未记录到比较器故障断言或 FAIL1 事件，动态关断与保持仍未验证。原图 05/06 已归档。先同步测 P0.0 与 OC 并以 UP 首沿触发，区分 GPIO 未出脉冲、接线传输及比较器输入问题。

## P1 动态激励波形定位

原图 07：CH1 PWM 周期 99.136 µs。CH3 有约 5 V、0.5 ms 平台，PWM 在此窗内及撤除后继续输出；CH2 无清晰对应平台。CH2/CH3 物理测点待用户确认。此次未捕获硬件关断，先核实探头位置及跳线两端；若 CH3 确在 OC，则读取同一次 before_release 快照并按 RAW/EIF/MOE 定位。

## P1 第二次同步波形

原图 08，按既定接线 CH1=UP、CH2=P0.0、CH3=OC。UP 周期 99.313 µs。CH2/CH3 在首个 PWM 后约 2 ms 同时出现约 0.5 ms 高脉冲，UP 此后未见后续 PWM，激励撤除后截图内约 0.5 ms 亦未恢复。可见部分支持 UP 关断及撤故障后保持。待同次 before_release/before_stop 的 RAW、EIF、MOE 和终态确认软件未提前介入。该图不定量判延迟，不证明当前高脉冲被截短或其余五路均通过。

## P1 UP 同次快照补证

撤除前 RAW=1、EIF=0x20、MOE=0；软件 STOP 前 RAW=0、EIF=0x20、MOE=0。两图均 FAIL012=0x420、TCLK=0x44。结合图08，支持 UP 在软件 STOP 前由硬件关断，且激励撤除后至关断前快照仍保持 MOE=0。两图未显示本次 state，不能独立确认完整观测时长或排除提前中止；保留首次未触发记录。六路、定量延迟和显式恢复仍待验。

- 2026-11-19_M2_09_P1_UP_before-release.png SHA256 f2dc53d9e355650335bfd2ba41c5496c64b029c78ab663b1031711fd2e21c77e
- 2026-11-19_M2_10_P1_UP_before-stop.png SHA256 e66b948747b3ec37d08ca84475fc04635136c32db9810dcddcfd61212eaaf38d

## P1 合回原工程后重建

用户确认原工程 Linko 07x Project 在 BENCH_TEST_CMP_FAIL,M2_P1_DYNAMIC 配置下完成 Rebuild，0 错误、0 警告。此项为用户文字确认。P1 UP 的实测证据沿用隔离版记录，P2 六路覆盖待实现及实测。


## P2 六路选择与实测口径（2026-11-19）

原工程 Linko 07x Project，宏 BENCH_TEST_CMP_FAIL,M2_P1_DYNAMIC。g_m2_p2_output=0..5 对应 UP/UN/VP/VN/WP/WN，RUN 时锁存到 g_m2_p2_active_output。每次只切换选中引脚到 MCPWM，其余保持 GPIO 安全态。P 路在故障前输出 PWM，故障后应低；N 路故障前持续低，故障后应高。保持独立核心板，P0.0→OC，双通道同步记录 OC 和所选 pad，OC 上升沿触发并保留 2 ms 以上预触发。每路复位后设置选择、确认 state=1、command=1；结束记录 state 和对应读回，检查软件 STOP 前的 MOE=0、EIF 含 0x20，以及目标引脚的电平/保持。保留 P1 首轮未触发和后续 UP 波形。P2 更新后先复核 UP 基准，再测其余五路；不能用未切到 MCPWM 的 GPIO 安全态替代该路验收。

原工程完整 Rebuild 为 0 错误、0 警告。P2 实测待执行。
HEX SHA256：af86c20ed34f4e9a5bafea0b7b9ebdbafd1cf8275f6524bd645f67e60a6da348。

## P2 U 相波形

按用户标注图11为UP、图12为UN，CH1显示目标引脚，CH2为同屏故障参考。UP故障前周期99.296 µs，故障到来后PWM停止，撤除后画面内保持低。UN故障前低，故障到来时转高，撤除后画面内保持高。两图为P/N安全态波形证据，未附这两次运行的终态或STOP前快照，未量化关断延迟。V/W四路待测。

- 2026-11-19_M2_11_P2_UP.png SHA256 00319f46368d74fe518ebba158b133c56d77e17b66b941f7d230371c230511ba
- 2026-11-19_M2_12_P2_UN.png SHA256 21ec3fd9623e5c54a15243ea3a0c8f230d0089327ea660b356704a9991deb939

## P2 与恢复验证结果

- P2 六路逐路功能检查 PASS。按操作者标注，P 路在 OC 有效后停止 PWM 并保持低，N 路转高并保持。六路 Watch 分别确认 active_output=0..5、state=3；before_stop 均为 raw_valid=1、raw=0、MOE=0、EIF=0x20。H2 在该独立核心板逐路测试及当前观测窗口内 PASS。
- UP 的 Manager 锁存与正常恢复路径 PASS。四份 Watch 快照依次为拒绝 RUN（PROTECTION_FAULT、FAULT_LATCHED、MOE=0、EIF=0x20），CLEAR（PROTECTION_OK、DISARMED、MOE=0、EIF=0），显式 RUN（PROTECTION_OK、RUNNING、MOE=1、EIF=0），最终 STOP（PROTECTION_OK、DISARMED、MOE=0、EIF=0）。故障再次注入及故障态复位仍待验证。
- H1 延迟初测 OPEN。CH1=UP、CH2=OC，时基 1 µs/div，OC 上升与 UP 下降的 50% 间距目测约 0.5 µs。该照片没有游标读数，且未用对照排除最后一个 PWM 脉冲自然结束。单帧估读不能建立最坏延迟或正式限值。延迟上限、最小故障脉宽及相位覆盖仍待冻结。

### H1 UP 脉宽对照与延迟

同次采集由操作者读出倒数第二个 UP 高脉冲 24.49 µs、故障时最后一个 19.49 µs，缩短 5.00 µs（20.4%）。放大照片中时间游标 AX=-540 ns、BX=+10 ns、ΔX=550 ns。A 靠近 OC 上升起点，记录为游标间隔，未严格统一按两路 50% 取点。本次 UP 提前截断功能确认。H1 正式时限和最小故障脉宽验收仍为 OPEN。

### UP 恢复后再故障

独立核心板，CH1=UP、CH2=OC。新版执行 CLEAR、显式 RUN 和第二次 OC 注入。照片 f5a8e89f-c99b-412d-8750-213f2a3eb0d6 显示 UP 恢复 PWM 约 2 ms（周期 99.303 µs），随后 OC 升高约 0.5 ms，UP 停止输出，OC 回落后画面内保持低。操作者确认 g_m2_recovery_state=2。根据固件判据，第二次 OC 撤除后、软件 STOP 前 RAW=0、MOE=0、EIF 的 FAIL1 位仍置位。UP 恢复后再故障 PASS。故障态复位及 H1 正式时限和脉宽边界仍待验证。

### H1 UP 短故障脉冲

独立核心板，CH1=UP、CH2=OC。实测 OC 高电平宽度 9.66 µs（波形 bd9d3b05-e303-4616-87c3-44d3e679be10）。OC 上升时 UP 正在输出高电平，随后约 0.5 µs 拉低（画面估读），OC 回落后在画面内保持低。配套 STOP 前快照 fcfbdeed-0188-4816-b491-2ef678b44b15 显示 raw_valid=1、raw=0、MOE=0、EIF=0x20。本次 UP 相位下 9.66 µs 单次功能检查 PASS。最小可检脉宽、最坏关断延迟及跨相位重复性仍 OPEN。故障态复位按操作者决定跳过，结论 NOT VERIFIED。

第二个 UP 相位点 PASS。OC 在 UP 拉高约 23.25 µs 后有效，实测高宽 9.60 µs（波形 1c70a1c9、c350daa4）。UP 随后下降，OC 回落后画面内保持低。同轮 STOP 前快照 779e83a4 显示 raw_valid=1、raw=0、MOE=0、EIF=0x20。注入靠近正常 PWM 下降沿，本点用于确认 FAIL1 捕获及撤除后的保持。两个通过点为 9.66 µs 和 9.60 µs。H1 正式限值、最坏延迟及重复覆盖仍 OPEN。

## V0.2 版本检查点（2026-11-20）

本表前部为早期 RTT 检查记录。当前独立核心板测试结果见后续各节。六路安全态及 STOP 前保持、UP 锁存与恢复、恢复后再次保护、两个约 10 µs 短脉冲点已获得实测证据。H1 正式时限、最小可检测脉宽及完整重复覆盖仍 OPEN，故障态复位 NOT VERIFIED。当前主 Target 宏为 BENCH_TEST_CMP_FAIL,M2_P1_DYNAMIC，仅供隔离核心板测试。该检查点尚未获得生产硬件资格。
