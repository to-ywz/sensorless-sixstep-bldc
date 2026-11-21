# V02-PROT-001 内部保护链准备与首轮验收

> **2026-11-21状态说明：** M2核心功能已按用户决策关闭；H1定量性能转后续backlog，故障态复位为WAIVED_BY_USER，不重复安排。当前结论与M3仅规划的授权边界见[总计划](V02-PLAN-000-workplan.md)，实测复用[M2证据](../reports/2026-11-18_M2-RTT-evidence.md)。下文旧“当前状态”、BLOCKED/未实现/未构建/未实测及操作建议按各自历史检查点理解，不是最新配置的统一结论；特定配置功能通过不代表资料冲突、全部硬件资格或CBC验收已关闭。

## 历史归档：2026-11-17 时钟专项与实现检查点

依据：[MCPWM FAIL滤波时钟专项审计](../reports/2026-11-17_V02-01-clock-path-audit.md)。历史结论保留；本文旧有“时钟门控未确认/FCLK门控冲突”的笼统描述按以下三项分别理解：

- **MANUAL + SDK：SYS_CLK_FEN bit10为MCPWM0模块门控，已确认；bit4属于TIMER0。** 不得开启bit4作为MCPWM使能。
- **MANUAL：图14-6内部链路已确认**：`FCLK → MCPWM0_TCLK.CLK_EN[2] → CLK_DIV[1:0] → TCLK → FLT.FLT_CLKDIV[7:0] → filter clock → 16周期滤波器`。时基另受BASE_CNTx_EN[6/7]控制；CLK_EN不是独立滤波时钟源选择。
- **INFERENCE / OPEN：** 图14-6的FCLK无编号；其与FCLK[10]为同一网络属于较强推断，尚缺厂商直接确认。正文FCLK[4]高度疑似笔误但证据不足。图14-6二级方框1～16与位表1～256、图14-3/14-6门控位置差异均保持MANUAL INCONSISTENCY / OPEN，不自行裁定物理实现。

专项静态审计已完成归档；厂商确认与必要实测待处理，不能将模块门控、内部链路、入口编号统一写作“门控未确认”。P1仍BLOCKED：入口编号关系、CMP窗口关闭语义、模拟量程/共模/极性及持续FAIL恢复条件等未决项未关闭；不新增或执行硬件测试，不修改硬件配置，不解除安全门禁。

当前状态：DESIGN完成 / IMPLEMENTED PARTIAL / 三个新增C模块语法检查先前已通过 / Mock未执行 / NOT BUILT / 烧录未执行 / P0～P4全部NOT RUN；config_valid仍为0，RUN仍BLOCKED，A5不修改。旧文“尚无Bench入口/未实现”和旧检查点“compiler validation outstanding”属历史状态，以本段为准；当前Bench仅阻塞诊断入口，非完整注入测试实现。

本专项未发现需立即修改的MCPWM时钟配置错误。当前源码已实现SYS_CLK_FEN快照、MCPWM0 bit10使能/读回、初始化前访问保护、关闭态配置读回与硬件资格分离，以及CMP_RAW无效占位保护（仅静态源码确认，未运行验证）。`config_valid=0`、RAW无效，RUN/MOE置1仍BLOCKED。

FAIL1_EN、CMP/DAC初始化、EIF W1C清除和真实注入流程仍未实现；Mock未执行，Keil完整构建未完成。

**CLOCK PATH STATIC AUDIT DOCUMENTED / V02-01 IMPLEMENTED PARTIAL / NOT BUILT / NOT HARDWARE TESTED**。专项审计执行本身只读；后续提交`6dc46f3`包含部分固件实现与文档更新。


- Date: 2026-11-15；静态审计补充：2026-11-16
- 只读依据：[V02-00审计与未决项](../reports/2026-11-16_V02-00-static-audit.md)；本段测试步骤是2026-11-15规划时点的准备记录，当前已有阻塞诊断入口，但真实注入Bench和以下P0–P4步骤仍未实现/未执行。六路物理隔离是包括P0在内的共同前置条件；本轮不进行硬件动作。
- Status: TEST PROCEDURE PREPARED / DIAGNOSTIC ENTRY IMPLEMENTED PARTIAL / P0–P4 NOT RUN
- V02-01平台接口与锁存状态机设计已单独归档：[设计文档](V02-01-platform-interface-state-machine.md)。本文件继续负责P0–P4内部保护链测试步骤；设计交付不代表Bench已实现或测试已放行。
- 前序：A5隔离台架主要功能用户实测正常；六管电源循环已确认。输入电气峰值、纳秒瞬态、完整瞬态VGS非重叠等遗留项保留，不宣称V0.1所有安全门槛关闭。
- 目标：**作为双级保护的第一阶段实验，先在锁存模式**证明CMP1_RAW→MCPWM0_FAIL012.FAIL1→CH0/1/2硬件关断，不依赖主循环或中断先执行STOP。**最终V0.2交付仍以CBC逐波限流为常规主保护，严重故障锁存为第二级保护。**
- 条件：无主母线、无电机。**首轮控制板逻辑测试要求控制板输出与未供电LKS560输入可靠隔离**（例如两板均断电后物理分离且核对信号/供电连接），不能让MCU在驱动VCC=0V时直接对仍相连的HIN/LIN输出高电平；这可能违反未供电输入电压额定并反灌。仅控制板使用已核实5V；若J-Link继续供MCU，避免与板载5V同时供电。未满足供电隔离或正式手册例外条件时P1/P2 BLOCKED，不得试探。
- 总体计划和硬件门槛见[V0.2总工作包](V02-PLAN-000-workplan.md)。

## 为什么先做内部触发

无实际电流仍能验证比较器到PWM关断路径。仓库SDK声明CMP1正端可选DAC0（CMP1_SELP_DAC0=7），负端可选内部REF（CMP_SELN_REF=1）。将DAC置于参考上下两侧是候选方法；实现前必须复核REF来源/范围、DAC量程与稳定时间，读CMP1_RAW实际翻转，不能只凭常量认定电压或故障极性正确。不占用CMP0 BEMF，不改电流采样跳帽、不接实际电流，不需要电子负载或第二台电源。

该注入只覆盖内部比较器/保护路由，不覆盖shunt/OPA/接线及真实过流阈值。先验证路由与保持/恢复，之后再分别做ADC零点、小信号标定及实际采样链注入。

## 实现约束

1. 保留A0～A5互斥入口；`BENCH_TEST_CMP_FAIL`独立入口已加入main.c并与现有入口互斥，当前仅为阻塞诊断骨架；真实注入和P0–P4执行序列仍未实现。共享已有物理关断极性/板级映射，不复制三相输出算法。
2. MCU寄存器配置留Platform，Bench负责单次触发和状态报告，产品后续应复用正式保护配置模块。
3. 初始化默认MOE关闭、HIN低/LIN高；先核实比较器安全态和**MCU输出与未供电预驱输入隔离**，再允许逻辑PWM。保持六路Fail默认值和mask正确，禁止自动恢复（TR0_AEC=0）。
4. 配置CMP1 RAW作为FAIL1来源、使能与极性；冻结滤波/消隐设置并记录，不将中断输出误当RAW。CMP中断不承担紧急STOP。
5. 单次RUN、单次注入，故障保持且拒绝在故障仍有效时重新输出；清故障与再次RUN分离，不能靠RUN自动解除故障。
6. 设计一个有限的“不执行软件关断”观测阶段：触发后CPU继续运行且暂不写MOE/GPIO，读取硬件状态及引脚证据，随后超时安全STOP。这样区分硬件关断与软件补救；不使用断点暂停或CPU死循环做首轮证明。
7. 为注入前后保存CMP RAW、FAIL寄存器和MOE、故障状态、计数器及默认态快照。硬件是否清MOE或内部屏蔽输出须按手册及实测分别描述，不能只凭MOE位判断引脚状态。
8. 宿主测试验证状态优先级、非法/未清故障拒绝启动及关断极性；不能将宿主模型替代硬件保护实测。保留限时STOP和启动检查。

## 首轮验收：不增加同时测点

| 步骤 | 操作/观测目标 | 判断与限制 |
|---|---|---|
| P0 | PWM保持关闭，内部DAC切换两档，读CMP1 RAW | 明确比较器实际翻转及极性；不需示波器多测点 |
| P1 | 无故障时输出有限逻辑PWM，注入一次，先观察一路**已与未供电驱动器输入隔离**的MCU高侧输出 | 硬件路径先关闭引脚，软件延后补救；不是实际Gate验证；若隔离条件不满足则BLOCKED |
| P2 | 用同样单次测试轮流覆盖U/V/W及对应低侧默认态 | HIN低/LIN高，最多两个探头；无需六路同时看 |
| P3 | 保持注入，尝试RUN；撤除注入但不清故障；再显式清除与RUN | 故障有效不启动、撤除后不自启动、明确恢复才允许输出 |
| P4 | 保持六路物理隔离，完成逻辑重复性、窗口/恢复边界检查与证据归档 | 不给LKS560/功率级供电，不观察Gate；驱动扩展另行评审，不由P1推断 |

若要量关断延迟，必须有与注入同步且实际可观测的时间参考；软件写DAC的时刻不能直接等于模拟CMP翻转时刻。不承诺尚无手册/波形依据的ns阈值，不要求用户先寻找额外探头。

## 后续门槛

内部链路通过之后，仍需采样通道实际覆盖、阈值/误差、母线电压采样、电源限流/急停与A5电气遗留项复核。尚不安排连接电机，不把一次内部DAC触发称为逐波真实电流限流验收完成。

## 双级保护决策在本轮的落实

- **本计划P0–P3一律先测锁存行为，禁用未经验证的自动逐周期恢复。** 故障有效时拒绝RUN；撤除故障不自动重新开通；必须明确清故障、显式RUN。
- 通过本轮仅证明`CMP1 RAW → FAIL1`路径可硬件关闭逻辑输出，**既不是CBC，也不是最终“每过流一次锁存停机”的产品决策**。
- 后续CBC须单独证明本周期截断、下一安全PWM周期恢复、不在本周期重新导通、连续过流的受控行为；严重/持续异常须优先升级锁存并禁止自动恢复。
- 现有CMP1输入通路和MCPWM FAIL自动恢复机制能否同时满足两级需求仍**TBD**。不得假定仅改变`TR0_AEC`就能实现安全CBC。参见[正式设计基线](V02-PLAN-000-workplan.md)和[完整验收](V02-PROT-001-cycle-current-limit.md)。

## 本轮与完整逐波限流的明确分界

P0/P1/P2/P3证明的是内部CMP/FAIL路径与逻辑输出的安全状态；若通过，仍不能宣称具备六扇区有效电流的cycle-by-cycle OCP。后续必须独立证明：采样源/有效PWM窗口、比较器实际模拟阈值、硬件故障后默认态、周期内关断与恢复的触发关系、各相保护覆盖，以及A5失电和真实VGS边沿安全问题。任何阶段不得通过直接短接或外接高压制造过流。

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
