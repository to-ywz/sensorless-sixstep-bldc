# V0.2 — 采样可信性与硬件过流关断：阶段执行计划

## 当前总控状态（2026-11-21）

本节是当前进度、M3规划和性能backlog的唯一维护入口；下文原阶段计划、2026-11-17检查点及其“下一步”均按历史时点保留，不再作为当前待执行清单。

依据：用户在「项目审核」接受M2核心关闭和H1性能项后移，并在本次仓库一致性更新中再次确认；实测事实复用[既有M2证据](../reports/2026-11-18_M2-RTT-evidence.md)，软件证据复用[WP1 Mock 45/45](../reports/2026-11-18_V02-01-software-mock-regression.md)、[WP2主Target构建](../reports/2026-11-18_V02-02-keil-primary-build.md)及[WP3-A静态审核](../reports/2026-11-18_WP3-A-protection-chain-static-review.md)。本次不重复审核或测试，不生成新硬件证据。

| 项目 | 当前结论 | 范围/依据 |
|---|---|---|
| M1保护链静态审核 | 已完成，复用 | WP3-A；不要求重审 |
| M2硬件保护链 | **核心功能关闭（CORE CLOSED）** | 外部CMP1输入→FAIL1、H1功能关断、H2保持、S1锁存、R1正常显式恢复及六路pad安全态已有证据 |
| H1定量性能 | **DEFERRED，见下方backlog** | 最坏响应延迟、最小可检测故障脉宽未确定，不阻塞M3规划 |
| 故障态复位 | **WAIVED_BY_USER / NOT VERIFIED** | 用户明确豁免，不记PASS，不重复安排 |
| M3电流采样 | **方案商讨中 / PLANNING ONLY** | 三相ADC/OPA可信性、PWM同步采样、六扇区电流可观测性；未经用户批准不执行硬件测试 |
| V0.2整体 | **未完成、未放行** | M2关闭不等于三相真实过流、CBC、功率级或电机运行验收通过 |
| A5功率级安全 | 既有PARTIAL PASS及风险保留 | HW-001/HW-002、实际双VGS非重叠等不因M2关闭而消失 |

M2关闭是明确的项目范围调整，**不是追认原R1全部判据已实测通过**。R1中的恢复竞争等完整边界覆盖未由本次重新核实；正常恢复与二次故障结论只适用于既有UP记录。M2-A/B/C/D原计划保留，不分别补造全面PASS或补签RTT READY。RTT早期NOT READY与后续实测并存的历史留痕不被改写成一次完整的正式就绪签署。

### M3规划范围与拟交付（尚未批准执行）

沿用本文件V02-02与V02-03工作包，不另建重复M3方案或测试报告；M3是V0.2内部里程碑，不是V0.3启动阶段。

| 方向 | 方案需明确的内容 | 拟交付/待冻结判据 |
|---|---|---|
| 三相ADC/OPA测量可信性 | U=R15/OPA2、V=R16/OPA1、W=R17/OPA0；核对实际ADC通道、参考、零点、增益、极性、共模、饱和、线性和串扰 | 每相校准表、原始码到电流公式及误差预算；数值指标TBD |
| PWM同步采样 | MCPWM→ADC触发、计数位置、采样保持/转换时长、边沿后建立时间、消隐及不同占空比下窗口 | 触发时序图与有效窗口定义；时序/占空比范围TBD |
| 六扇区电流可观测性 | 按现有六步表区分导通、关断、续流、换相重叠与盲区；逐相标记有效/无效数据 | 六扇区×PWM状态的可观测矩阵、适用条件与盲区处理；不能把ADC重构当异步OCP覆盖 |

规划输入统一引用[采样映射](../../hardware/boards/lks32mc071/sampling.md)和[保护资源限制](../../hardware/boards/lks32mc071/protection_chain.md)：P-I三电阻方案已确认，但P5/P6/P7实物跳帽、OPA前端串联电阻/增益、ADC实际映射及动态响应仍需确认。R69/OPA3约2ms/80Hz是简化电路估算，不是带宽实测，更不代表完整母线瞬时电流；CMP1一次仅选一路，不能推定三相硬件OCP覆盖。

**当前仅整理方案、依赖与验收草案。** 不烧录、不上电、不执行硬件测试；后续先完成M3方案商讨、冻结适用配置/判据及现场条件，再由用户明确批准执行。既有M2授权不延伸到M3。CBC实现、真实功率级/电机运行以及A5全面整改不扩大为本轮M3规划任务；V0.2双级保护长期目标保留。

### 后续性能优化backlog（无执行排期）

| ID | 状态 | 已有证据与待完善内容 | 依赖/边界 |
|---|---|---|---|
| H1-PERF-01 | DEFERRED，非M2关闭/M3规划阻塞项 | 最坏响应延迟：UP脉冲24.49→19.49µs证明提前截断；550ns仅为既有游标间隔，非正式最坏值 | 后续性能优化时先冻结参考取点、时限与必要相位/重复条件；另行批准测试 |
| H1-PERF-02 | DEFERRED，非M2关闭/M3规划阻塞项 | 最小可检测故障脉宽：现有UP两点9.66µs与9.60µs通过；9.60µs不是最小边界 | 后续明确脉宽/相位覆盖与检出标准；复用已有两点，不重跑整套M2 |

故障态复位属于豁免记录，不进入待补测backlog。上述性能结论仍须在后续正式保护性能验收前完善；M2核心关闭不放宽V0.2最终安全要求。

### 本次仓库一致性扫描记录

- 基线：默认分支main，扫描前HEAD `05468ef361bf191c04e7d03c6cc01870355736eb`，提交 `test(protection): checkpoint V0.2 M2 hardware validation`，北京时间2026-11-20 23:16:51。
- 范围：完整递归文件树（响应未截断）、98份项目Markdown（排除SDK/厂商手册正文），重点核对README/CHANGELOG、架构/需求、路线与V0.2计划、测试用例/报告、板卡采样/保护/信号映射及硬件问题。未进行固件代码审查、构建或硬件复测。
- 远端可见分支：main与`work/v02-01-implementation-20261116`（`2049c6e7be4b673873adf4d21b75a75998633029`）；只核对分支头，不将旧分支自动合并或删除。Issues查询（全部状态）及开放PR查询均返回空；GitHub Milestones端点不受当前连接器支持，无法确认平台上的里程碑状态，本文M1/M2/M3为项目文档状态。
- 本地当前目录本身不是Git仓库。`v02-audit-repo`位于旧HEAD `d99888d66480160f12bec898ee9389797048b2ac`，有未提交Keil工程/构建/RTE与未跟踪日志等文件；`v02-doc-review`位于`44dfabed03338bee537468c0079c0d6beec0dcef`，相对本地origin/main领先1个提交，工作树干净。该比较基于本地缓存，未冒充远端最新关系。两者均未修改。其他导出/构建目录及仓库外的实际Keil工作区状态未全面核实。
- 发现并修正：README/总计划/入口仍停留“未实现/未测试”；RTT页首历史NOT READY误作当前结论；缺M2核心关闭、豁免及H1后移记录；缺M3规划边界及backlog入口。更新现有入口和历史说明，不新建平行进度表，不搬迁或删除原始报告。
- 保留事项：架构文档为目标架构，无需为本次进度变化改写；CBC尚未实现/未验证的最终验收要求保留；A5/HW-001/HW-002及下一版PCB候选保留。旧设计与检查点正文作为历史归档，仅增加适用时点说明；供应商资料冲突仍按原审计记录保留，不因特定配置功能通过而全量关闭。
- 证据限制：本次复用仓库脱敏文字、哈希/截图索引与用户确认，未重新查看或核验全部私有原始波形、最终板载固件指纹；不推测生产硬件资格，不把M2结论推广到真实电流阈值、全部异常竞争边界或功率级。



## 历史归档：2026-11-17 时钟专项与实现检查点

依据：[MCPWM FAIL滤波时钟专项审计](../reports/2026-11-17_V02-01-clock-path-audit.md)。历史结论保留；本文旧有“时钟门控未确认/FCLK门控冲突”的笼统描述按以下三项分别理解：

- **MANUAL + SDK：SYS_CLK_FEN bit10为MCPWM0模块门控，已确认；bit4属于TIMER0。** 不得开启bit4作为MCPWM使能。
- **MANUAL：图14-6内部链路已确认**：`FCLK → MCPWM0_TCLK.CLK_EN[2] → CLK_DIV[1:0] → TCLK → FLT.FLT_CLKDIV[7:0] → filter clock → 16周期滤波器`。时基另受BASE_CNTx_EN[6/7]控制；CLK_EN不是独立滤波时钟源选择。
- **INFERENCE / OPEN：** 图14-6的FCLK无编号；其与FCLK[10]为同一网络属于较强推断，尚缺厂商直接确认。正文FCLK[4]高度疑似笔误但证据不足。图14-6二级方框1～16与位表1～256、图14-3/14-6门控位置差异均保持MANUAL INCONSISTENCY / OPEN，不自行裁定物理实现。

专项静态审计已完成归档；厂商确认与必要实测待处理，不能将模块门控、内部链路、入口编号统一写作“门控未确认”。P1仍BLOCKED：入口编号关系、CMP窗口关闭语义、模拟量程/共模/极性及持续FAIL恢复条件等未决项未关闭；不新增或执行硬件测试，不修改硬件配置，不解除安全门禁。

当前状态：DESIGN完成 / IMPLEMENTED PARTIAL / 三个新增C模块语法检查先前已通过 / Mock未执行 / NOT BUILT / 烧录未执行 / P0～P4全部NOT RUN；config_valid仍为0，RUN仍BLOCKED，A5不修改。旧文“尚无Bench入口/未实现”和旧检查点“compiler validation outstanding”属历史状态，以本段为准；当前Bench仅阻塞诊断入口，非完整注入测试实现。

本专项未发现需立即修改的MCPWM时钟配置错误。当前源码已有部分平台有效性保护（仅静态核对，未运行验证）：SYS_CLK_FEN快照、MCPWM0 bit10使能/读回、初始化前访问保护、关闭态配置读回与硬件资格分离、CMP_RAW无效占位保护。`config_valid`和`hardware_qualified`仍为0，RAW仍无效，RUN/MOE置1仍BLOCKED；详见[平台接口README](../../firmware/platform/lks32mc071/README.md)。

未完成项仍包括CMP/DAC初始化、FAIL1_EN、EIF W1C清除、真实Bench注入序列、Mock测试和完整Keil构建。

**CLOCK PATH STATIC AUDIT DOCUMENTED / V02-01 IMPLEMENTED PARTIAL / NOT BUILT / NOT HARDWARE TESTED**。专项审计执行本身只读；后续提交`6dc46f3`同时包含部分固件实现与文档更新。


- **规划日期：2026-11-16**
- **Status（2026-11-16规划时点）：V02-00 STATIC REVIEW COMPLETE / SOURCE CONFLICTS OPEN；V02-01 DESIGN PREPARED / IMPLEMENTATION NOT STARTED / NOT BUILT / NOT RUN；后续阶段 NOT STARTED。当前状态以本文2026-11-17归档摘要及实现检查点为准。**
- **审计依据与当前下一任务：** [2026-11-16 V02-00记录](../reports/2026-11-16_V02-00-static-audit.md)。V02-01接口/状态机设计见[独立设计文档](V02-01-platform-interface-state-machine.md)。附件全文、原版窗口图、FCLK编号冲突及AEC更新事件仍开放；本轮不执行硬件动作。
- **范围**：LKS32MC071CBT8 + LKS_EVB_MVPOWPRE_V5.0；先无主母线逻辑级保护，再验证低能量模拟采样；最后依据A5和硬件条件决定是否开展功率级验证。**本计划不是已完成的测试报告**。
- **当前已知**：A5隔离台架低/高侧基础功能有证据，但[整体仍PARTIAL PASS](../reports/2026-11-15_V01-A5-progress.md)，HW-001/HW-002及实际双VGS非重叠安全门槛未关闭。**不连接电机，不给主功率母线上电，不以短路制造过流。**
- **隔离约束**：首轮称作“控制板逻辑级”仅在**MCU输出与未供电LKS560 HIN/LIN之间确认物理隔离并保留断电通断证据**时成立。不能一边给MCU上5V驱动高电平，一边让仍相连的LKS560 VCC=0V；否则可能超过未供电输入额定、反灌驱动器。不得在带电状态拆插两板。

## 0A. V0.2 正式设计基线：逐波限流为主，故障锁存兜底（2026-11-16 决策确认）

**最终交付不是“发生一次过流就停机”，也不是“只要CMP1能触发FAIL1就算逐波限流实现”。** 本阶段采用两级保护目标：

| 保护级别 | 触发性质 | 硬件动作 | 恢复规则 | 当前实现状态 |
|---|---|---|---|---|
| **一级：Cycle-by-Cycle（CBC）逐波限流** | 每一有效PWM周期内经可信模拟电流路径达到限流条件 | **截断本周期的有效导通**；不得等CPU轮询才停 | 仅在设计并实测的**安全PWM周期边界**、保护条件允许且没有二级故障时重新允许下一周期；不能让持续故障造成失控反复导通 | **设计目标；NOT IMPLEMENTED / NOT TESTED** |
| **二级：严重故障锁存（Latched Fault）** | 严重峰值、持续多个周期受限、保护源异常/不可信、关键供电或驱动安全异常等**候选**条件 | 关闭危险驱动请求并保持故障态；禁止自动重启 | 先确认故障源撤除及安全态，再明确清故障、显式重新RUN；不得随下一PWM周期自行恢复 | **软件状态机骨架 IMPLEMENTED PARTIAL；硬件FAIL锁存/清除链 NOT IMPLEMENTED / NOT TESTED** |

**执行顺序与最终架构不同**：V02-01先以**故障锁存模式**验证`CMP1_RAW → FAIL1 → CH0/1/2`的硬件异步关断能力、物理输出安全态及显式恢复；这是安全基础实验，**不是把锁存停机选为最终主要限流策略**。随后V02-02/03验证模拟链及六扇区覆盖，V02-04才研究CBC本周期截断和受控周期恢复，并把二级锁存集成进故障状态机。

**尚未证明的架构条件（必须在V02-00/03确认）：**
1. 现有CMP1一次只选一路输入；三电阻采样不等于六扇区硬件OCP覆盖。CMP0仍保留给BEMF，不能未经审核挪用。
2. 两级保护**逻辑上分开**，但不能预设有两路独立模拟比较器、能同时设置两种阈值，或一条FAIL1既天然CBC又天然锁存。严重故障触发源、阈值检测、锁存通路与CBC恢复机制，须依据LKS32MC071真实寄存器/板卡资源实测选型；资源不足时**记录硬件限制，评估额外保护器件或下一版PCB**，不能用软件中断假冒异步保护。
3. `TR0_AEC`等自动恢复能力、故障极性、Fail输出模式与计数器同步语义尚未验证；CBC启用前必须证明**本周期不会再次开通、下一安全周期才允许恢复**。
4. CBC阈值、严重故障阈值、持续次数、blanking、延时和恢复条件均为**TBD**；不在当前文档硬编码经验值。
5. 二级故障优先级高于CBC；锁存期间禁止任何周期性自动重开。CPU可记录和管理状态，但**首级真实过流截断不能依赖CPU响应时序**。

**验收边界：** V02-01锁存逻辑测试通过，只能标`INTERNAL_FAIL_PATH_PASS`（若确有实测）；V0.2整体通过还需可信电流感知、六扇区覆盖、真实CBC截断/恢复以及严重故障锁存的互斥与升级策略证据。详见[最终逐波保护验收](V02-PROT-001-cycle-current-limit.md)。

## 0. 设计原则：先证明“能关”，再证明“凭什么关、何时关”

```text
内部可控测试激励（候选 DAC0 / REF；必须实核）
        ↓
   CMP1 RAW 实际翻转
        ↓
 MCPWM0_FAIL012.FAIL1（来源选择、使能、极性、mask）
        ↓
 CH0/CH1/CH2 物理输出进入 LKS560 安全逻辑态
        ↓
 [后续另测] 驱动器和真实 VGS
        ↓
 [再后续] 可信且覆盖六扇区的模拟电流链
```

- 分工不变：**CMP0 给 BEMF，CMP1 优先给 OCP**；不恢复旧“拆Hall电阻/电容、BEMF改接CMP1”的错误路线。候选内部通路是 `CMP1_RAW → MCPWM0_FAIL012.FAIL1 → CH0/1/2`，参见[硬件保护链](../../hardware/boards/lks32mc071/protection_chain.md)。
- **双级目标已确定**：常规可控过流优先采用逐波截断/安全周期恢复；严重或持续异常采用锁存保护。**首阶段以锁存方式证明硬件能关断**，自动按PWM周期恢复仅作为后续独立实验，不得预先开启。
- 采样有效性先于保护阈值。当前三相 5mΩ 低侧分流电阻与公共R69均有**不同的导通/续流可见窗口**；CMP1一次只选一个输入，不能默认一条OPA2-U链保护全部六扇区。没有完整覆盖方案时须记录阻断项，不能将单相测试升级为全桥硬件OCP。
- 任何安全阈值、故障延时、blanking、ADC采样时刻须依据正式手册、MCU时钟、功率器件额定及受控实测后冻结；**本阶段不编造ns阈值或mV/A放大倍数**。

## 1. 分阶段工作包与验收门槛

| 阶段 | 目标与任务 | 测量/证据 | 过关标准与失败处理 |
|---|---|---|---|
| **V02-00 资源/接线冻结** | 核对芯片修订与原版寄存器；CMP1 RAW、DAC0/REF 的实际选项；FAIL1_SEL、FAIL1_EN、极性、故障输出默认、mask、TR0_AEC、引脚复用；断电核对 P5/P6/P7 选P-I和 R69/PGND/GND 回路 | 寄存器-手册-源码交叉表、断电照片/通断结果、信号映射 | 每个设置有来源；不确定的硬件供电/默认态不进入动态注入 |
| **V02-01 内部比较器+FAIL链** | 单独Bench入口；驱动电源禁用且MCU到未供电驱动器输入已可靠隔离、主母线0V、无电机；先DAC/REF使CMP1 RAW翻转，后启用FAIL1并有限时逻辑PWM；验证无需软件先STOP即可关断 | CMP RAW、FAIL/MOE寄存器、MCU六路引脚分组波形、故障/清除状态、构建指纹 | 注入前后逻辑态对应，故障时物理输出为**HIN=0、LIN=1**；不依赖主循环/中断。无效输入或失效路径立即停止测试并定位，不飞线兜底 |
| **V02-02 ADC/OPA静态可信** | 不接电机、无功率输出；校验ADC参考/零点/增益/极性/转换公式；逐个核对OPA2(U)、OPA1(V)、OPA0(W)与OPA3(R69)映射、饱和/共模；DM_In不是主BUS读数 | 已知受控安全小信号与ADC原始码、OPA原始值、DMM对照、校准表 | 每条链实测线性与零点、单位和采样基准；误差容限先经器件分辨率及应用要求定义。若失真，停止基于该通路计算阈值 |
| **V02-03 PWM采样窗口与六扇区覆盖** | 按120°六步状态逐扇区建“通路可见/盲区/续流/消隐”矩阵；审计R15/R16/R17、R69与CE1/CE2的电流回路；验证ADC触发与实际PWM计数同步、采样保持时间 | 6×(导通/关断/续流)采样有效表、真实或经验证的受控低能量注入数据、时间戳 | 能区分有效窗口和盲区；CMP1可见的保护通路**必须覆盖每个危险扇区**，否则本轮不能宣称三相逐波保护，需评审硬件/调制方案 |
| **V02-04 CBC逐波截断 + 二级锁存策略** | 先确认严重故障锁存/显式恢复，再证明CBC触发后仅截断本PWM有效段、在安全周期边界受控恢复；验证持续限流、升级锁存及锁存优先级；禁止一开始自动恢复 | 故障状态表、受控模拟时序、异常恢复/故障保留回归 | 严重故障锁存时绝不自动重启；普通CBC按安全周期边界恢复，且不出现同周期再开启或异常连续重开；任一条件不具备即不放行CBC |
| **V02-05 功率级门槛（仅预留）** | 需先关闭A5有关安全门槛并建立可靠保护和供电/限流；另立低能量功率试验计划 | 实物安全审核签署、VGS与电流同步证据 | **当前BLOCKED**；未经明确验收，不安排电机、母线带电、负载或真实短路试验 |

> 资源映射以[采样链路文档](../../hardware/boards/lks32mc071/sampling.md)为准：U=R15/OPA2、V=R16/OPA1、W=R17/OPA0；R69=OPA3。P5/P6/P7是P-X/P-I信号选择，**不是**单/双/三Shunt功率拓扑切换。下一版PCB可选模式见[next_revision](../../hardware/boards/lks32mc071/next_revision.md)，不混作本版V0.2能力。

## 2. V02-01 首轮操作拆分（优先执行）

按[详细的内部保护链P0-P4计划](V02-PROT-001-internal-path-plan.md)执行；下面是交付验收的最小阶段门槛。

1. **只读核查**：V1.70手册原始表、SDK V3.16、芯片修订和当前MCPWM默认设置；先校验FAIL1选择的是CMP1 RAW，不受SDK错误注释影响，明确有效电平及触发后的真实输出。
2. **构建安全Bench**：互斥入口`BENCH_TEST_CMP_FAIL`及诊断骨架已加入；目前只输出阻塞状态/快照，真实DAC注入、有限逻辑PWM和完整P0–P4测试序列**尚未实现**。任何后续硬件测试仍要求驱动器不通电且逻辑输出与未供电HIN/LIN物理隔离、主母线0V、无电机。平台层负责CMP/DAC/FAIL寄存器，Bench层管理注入与状态，复用已有LKS560物理关断极性，禁止复制六步电机控制逻辑。
3. **P0 只验证CMP1翻转**：不打开PWM时，测RAW随着两档受控参考的变化及恢复；若DAC0/REF路径、共模/量程无法确认，先停下用可读寄存器查因，不用未核对的高压外部信号强行注入。
4. **P1 验证硬件切断**：有限时逻辑PWM运行时触发RAW，先观测MCPWM的物理 HIN/LIN 是否进入0/1；记录硬件动作与软件安全兜底的先后，不让CPU中断/轮询先代替保护。普通双通道探头分U/V/W逐次检查，**同次测量时将一通道用于故障可见参考**，否则不能量化真实硬件延时。
5. **P2-P3 错误与恢复**：三相六路逻辑态、持续故障RUN拒绝、故障撤销但未清除仍拒绝、显式清除后方允许下一次RUN；动态自动恢复不在首轮放开。
6. **P4 隔离逻辑归档**：继续保持六路MCU输出与LKS560输入物理断开，仅归档逻辑重复性、边界与恢复证据；本轮不安排驱动/Gate或功率级供电。未来扩展必须另立计划。

### 必须避免的误判

- `CMP1_RAW_DATA`翻转 ≠ 已成功关断PWM；`MOE=0` ≠ 已测到正确物理脚电平；软件执行STOP ≠ 硬件异步保护。
- `VGS`最终0V ≠ 电源缓降期间快速安全关断；HIN/LIN无双请求 ≠ 六管真实VGS非重叠。
- `Vol_ADC`测到DM_In ≠ 当前P3断开状态下真正+48V BUS电压。
- 单次DAC模拟触发 ≠ 已标定ADC、测到真实过流或证明所有六步扇区覆盖。
- R69的RC约2ms（理想差分近似、截止频率约80Hz），不能在没有额外带宽实测的情况下承担逐PWM快速保护。

## 3. 软件结构（方案，不是已提交代码）

```text
firmware/
  platform/lks32mc071/  CMP1 / DAC / MCPWM FAIL 寄存器适配
  board/lks_evb_mvpowpre/   HIN/LIN默认态、模拟通道、阈值标定配置
  tests/bench/              独立 V02-01 注入控制、状态报告与超时STOP
  tests/unit/               RAW极性、故障优先级、锁存/恢复、超时/互斥回归
  motor/                    后续正式过流保护策略（此阶段不改电机闭环）
```

产品代码与台架共用**平台保护配置接口**，Bench不直接散布寄存器魔数。仅加入必要诊断变量，例如RAW、FAIL、MOE、stage、fault、completed、timing_valid；这些字段的最终命名以实现时审查为准。**不要为诊断复制现有MCPWM控制后端，也不要修改A5已确认波形的参数。**

## 4. 交付清单与版本策略

| 交付 | 是否已有 |
|---|---|
| 方案文档与测试矩阵 | 本文为**计划** |
| PROT-001内部Fail P0-P4计划 | [已有准备稿](V02-PROT-001-internal-path-plan.md)；计划已归档，诊断入口IMPLEMENTED PARTIAL，真实注入序列NOT IMPLEMENTED / NOT TESTED |
| 芯片寄存器与保护极性审计记录 | [静态审计已归档](../reports/2026-11-16_V02-00-static-audit.md)；来源冲突开放，非硬件验收 |
| V02-01 平台接口/锁存状态机设计 | [设计已归档](V02-01-platform-interface-state-machine.md)；平台/Manager/Bench已有部分代码，IMPLEMENTED PARTIAL / NOT BUILT / NOT TESTED |
| Keil隔离构建及编译/HEX SHA | TODO |
| P0 CMP RAW、P1-P3三相逻辑保护波形与状态 | NOT TESTED |
| ADC/OPA标定及六扇区采样/故障覆盖矩阵 | TODO |
| 周期内关断与恢复、功率级低能量验证 | BLOCKED（前置未满足） |

完成每个工作包再提交对应文档与测试证据；可在V0.2范围内做开发快照，但**不能对V0.2整体打PASS/Tag放行**，直到保护、采样、恢复和安全门槛均得到真实验证。原始波形默认留私有raw，公开仓库只提交脱敏文字、参数和哈希。

**下一步具体行动：先补齐Protection Manager/Platform的Mock回归（状态优先级、故障与RUN竞争、CLEAR失败、重复初始化等），再完成隔离输出目录下的完整Keil构建/链接并归档日志。并行向厂商确认图14-6入口FCLK编号；CMP窗口关闭语义、DAC0/REF量程/共模/极性及持续FAIL恢复优先级仍须单独关闭。P0–P4目前全部NOT RUN；P1保持BLOCKED，未满足物理隔离与前置证据前不启动实机测试。** 不需要为了“赶V0.2”上主功率母线。

## V02-01 implementation checkpoint (2026-11-17)

Status: DESIGN retained / IMPLEMENTED PARTIAL / NOT BUILT / NOT TESTED.

New platform: `lks_protection.h/.c`; application: `protection_manager.h/.c`;
independent entry: `BENCH_TEST_CMP_FAIL`, `bench_cmp_fail.h/.c`.
SDK and A5 backend files are unchanged. Existing user Keil settings are retained.

The platform uses SDK MCPWM initialization and existing board off defaults.
It saves boot EIF before SDK initialization clears it. Defaults are CH_DEF[5:0]=0x15,
CH_MASK[5:0]=0x3F, CLK_DIV=0, FLT=0, AEC=0, AUEN=0, CNT0=0, MOE=0.
FAIL1_SEL=1 is prepared, but FAIL1_EN remains zero. No SYS_CLK_FEN bit4 write.
Sources: SDK `Source/lks32mc07x_mcpwm.c`, existing `lks560_pwm_defaults.c`,
manual MCPWM FAIL012/CH_DEF/CH_MASK/TCLK/SDCFG/EIF/PRT tables.

FCLK numbering, disabled-window bypass, qualified analog reference/polarity,
and sustained-FAIL restart semantics remain BLOCKED. Platform RUN and event-clear
return BLOCKED; no unqualified DAC code or CMP initialization is applied.
W1C hardware clear, qualified CMP/DAC setup and actual injection sequences are
NOT IMPLEMENTED. Bench reports each hardware case BLOCKED via debugger-visible
`bench_cmp_fail_report`; it is a blocked diagnostic entry, not a completed P0-P4 bench.

Manager provides BOOT_SAFE, CONFIGURING, DISARMED, RUNNING, FAULT_LATCHED and
CLEAR_CHECK; synchronous requests serialize with saved/restored PRIMASK.
Fault latch takes precedence over RUN and STOP; reinitialization cannot erase it.
CLEAR does not start output. Platform configuration blocking prevents DISARMED/RUN.
Mock regression tests are still outstanding. The three new C modules passed ARM Compiler C99 `-fsyntax-only` syntax checks; this is not a full Keil build or link. Full Keil build/link and hardware validation remain outstanding.
All P0-P4 hardware tests remain NOT RUN; no programming or hardware power actions.
Do not interpret this checkpoint as V02-01 acceptance or hardware protection qualification.
