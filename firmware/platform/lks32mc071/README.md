# LKS32MC071 Platform

> **2026-11-21状态说明：** M2核心功能已按用户决策关闭；H1定量性能转后续backlog，故障态复位为WAIVED_BY_USER，不重复安排。当前结论与M3仅规划的授权边界见[总计划](../../../tests/test_cases/V02-PLAN-000-workplan.md)，实测复用[M2证据](../../../tests/reports/2026-11-18_M2-RTT-evidence.md)。下文旧“当前状态”、BLOCKED/未实现/未构建/未实测及操作建议按各自历史检查点理解，不是最新配置的统一结论；特定配置功能通过不代表资料冲突、全部硬件资格或CBC验收已关闭。

实现 MCU 相关外设适配：时钟、MCPWM、ADC、CMP、Timer、GPIO、IRQ、Watchdog。

要求：
- 封装寄存器访问。
- 提供可测试的初始化和状态读取接口。
- 明确 PWM 安全默认态。
- 明确 ADC 触发源、采样时间和中断优先级。
- 明确 CMP 到 PWM Break / Driver Disable 的硬件路径。
- 不在此层实现电机启动、BEMF 算法或速度 PI。


## V02-01 保护接口边界（设计已归档；固件部分实现）

CMP1 / MCPWM FAIL 的寄存器操作、状态快照、FAIL事件清除和MOE控制属于本平台层；电机运行状态、故障锁存策略和显式RUN/CLEAR命令属于Application层。后续应由Protection Manager成为唯一允许请求MOE开启的上层入口，Bench与产品逻辑复用平台保护接口，不允许A5或其他输出后端绕过故障锁存直接写MOE。

设计文档：[V02-01平台接口与锁存状态机](../../../tests/test_cases/V02-01-platform-interface-state-machine.md)。当前已有`lks_protection.h/.c`、`protection_manager.h/.c`及`bench_cmp_fail.h/.c`部分实现；本README同时记录实现边界，不代表保护链已验证。A5后端未修改；当前仍未完成Mock、完整Keil构建或硬件测试。

## V02-01 implementation checkpoint (2026-11-17)

Status: DESIGN retained / IMPLEMENTED PARTIAL / NOT BUILT / NOT TESTED.

New platform: `lks_protection.h/.c`; application: `protection_manager.h/.c`;
independent entry: `BENCH_TEST_CMP_FAIL`, `bench_cmp_fail.h/.c`.
SDK and A5 backend files are unchanged. `LK_StdPeriph.uvprojx` was updated in the implementation checkpoint to add the V02 Protection group and three C sources to both Targets; user-specific Keil settings remain separate from this tracked project-file change.

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


## 2026-11-17 接口有效性增量完善

IMPLEMENTED PARTIAL，RUN仍BLOCKED，未改变时钟/模拟硬件资格门禁。

- 快照新增sys_clk_fen、mcpwm_readable、raw_valid、safe_config_matches、hardware_qualified。
- Read(NULL)不操作；初始化前或MCPWM模块门控关闭时只返回系统时钟状态，不访问MCPWM/CMP。
- raw_valid当前恒0，raw=0仅为占位，不代表无故障；Manager拒绝使用无效RAW放行RUN或CLEAR。
- SetMoe(0)允许初始化前调用：先用SDK使能并读回bit10，失败则返回IO_ERROR，不访问MCPWM；关MOE/CNT0后在临界区内核对两者。
- safe_config_matches只表示当前关闭态的寄存器目标匹配，不代表FAIL已使能或保护可运行。hardware_qualified和config_valid仍为0；Init(NULL)返回INVALID。
- SDK和A5后端未修改；当前实现检查点的Keil工程已登记V02 Protection分组及三个C源文件，未增加bit4操作。W1C/CMP/DAC/完整注入仍待实现。
- 三个模块已通过实际ARM编译器C99语法检查，SDK文件名大小写警告仍存在。Mock执行、完整Keil构建和硬件测试均未完成；本轮没有固件产物。
