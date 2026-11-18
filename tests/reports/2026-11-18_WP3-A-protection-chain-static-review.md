# WP3-A：V0.2 保护链整体静态审核

- 日期：2026-11-18；审核类型：一次性整体只读代码审核。
- 本地仓库：F:/Demo/sensorless-sixstep-bldc，分支main。
- HEAD及通过git ls-remote读取的远端main均为c1791bd4542abf0c55a1f5af63079d84f095a9e7；未fetch/pull、未覆盖任何本地改动。
- 本轮只生成本文；未重跑Mock、未构建、未改代码/工程、未烧录、未操作硬件、未提交/推送。
- 审核对象为当前工作树，另对比HEAD工程宏；源码行号针对上述HEAD及本机工作树（生产C/H相同），工程宏分别标注。

## 1. 首要结论

**当前CMP_FAIL入口保守保持关闭：PASS（静态与已有软件证据限定）。端到端保护链及P0～P4实测就绪性：FAIL。**

真正阻断推进的是平台和Bench仍是部分实现，不是缺少更多Mock或重复构建：CMP/DAC及真实RAW未实现，FAIL1仅选源而未使能，受保护PWM/受控注入与硬件清故障未实现。当前门禁不能直接改成有效来绕过缺项。

复用WP1后，本轮未发现当前CMP_FAIL调用链中新证实的错误RUN/CLEAR或自动重启漏洞；这不等于已证明硬件竞态安全。旧Bench确有绕过Manager开启MOE的路径，但当前CMP_FAIL映像内已裁剪，不能误报为当前分支正在越权运行。产品级“全局唯一MOE权限”仍未成立。

**可进入：主控审阅后授权补齐实现和必要前置条件。不可进入：直接执行P0～P4、启用CBC、驱动/主母线/电机试验。** P0模拟比较器准备无需为了完整性先重做WP1/WP2；但执行实机P0仍须满足模拟配置和物理隔离门槛。

## 2. 版本、工作区及复用证据

开始时已有，且本轮保留：

```text
 M firmware/projects/keil/lks32mc071/LK_StdPeriph.uvoptx
 M firmware/projects/keil/lks32mc071/LK_StdPeriph.uvprojx
 M firmware/projects/keil/lks32mc071/Objects/LK_StdPeriph.hex
 M firmware/projects/keil/lks32mc071/RTE/_Linko_07x_Project/RTE_Components.h
?? firmware/projects/keil/lks32mc071/JLinkLog.txt
?? firmware/projects/keil/lks32mc071/RTE/_Linko_07x_Project_ac6/
```

暂存区为空。主Target工作树宏为BENCH_TEST_CMP_FAIL；HEAD/远端的同一字段仍是BENCH_TEST_GATE_DRIVER。第二Target已由用户排除，不以其历史失败阻塞主用Target，也不再审核修复它。

| 既有证据 | 本次复用结论 | 不扩大的范围 |
|---|---|---|
| tests/reports/2026-11-18_V02-01-software-mock-regression.md:12–40 | 45个独立场景、两轮回归及10种故意回退对照通过；真实Manager+Fake平台 | 不重跑，不代表真实平台或硅片保护通过 |
| tests/reports/2026-11-18_V02-02-keil-primary-build.md:10–16、22–30、49 | 主用Target真实模块编译链接成功，0错0警告；本地CMP_FAIL配置；35个OBJ | 不重建，不把Git默认A5配置或第二Target算同一构建 |
| tests/reports/2026-11-17_V02-01-clock-path-audit.md:12–115 | 模块门控bit10、内部CLK_EN/分频链已确认；图中FCLK入口编号等仍OPEN | 不重做原版图审计，不把已确认bit10写成未知 |
| tests/reports/2026-11-16_V02-00-static-audit.md:保护链及恢复章节 | RAW/窗口/FAIL极性、mask/default、AEC及隔离限制 | 仅沿用未关闭边界，不推断新硬件事实 |

当前main、Manager、Platform和Bench四个C文件与WP2隔离副本逐字节相同，已有结果可复用。WP2的IDE布局/缓存时间戳例外已在其报告说明，不重新创建一套完整性流程。

## 3. 实际调用链：在哪一段终止

当前本机主Target（BENCH_TEST_CMP_FAIL）：

```text
main.c:51 → board_init_safe()
  → board_init.c:61 / board_pwm_inputs_off()
  → GPIO物理HIN=0、LIN=1，保持AF0，随后设置48MHz RC

main.c:54 → bench_cmp_fail_init()
  → bench_cmp_fail.c:20 → Protection_Init()
    → protection_manager.c:43 → LksProtection_Init(&fault.boot)
      → lks_protection.c:106 → MCPWM0模块时钟bit10
      → :111 → SDK初始化清EIF前保存boot
      → :112 → LksProtection_SetMoe(0)
      → :116–120 → StructInit + FAIL1_SEL=1 + 默认态助手 + MCPWM_Init
         CFG仅准备关闭态；CMP/DAC未初始化，FAIL1_EN仍0
      → :123–125 → 验证关闭态子集后返回BLOCKED
    → protection_manager.c:46 / latch()
         请求MOE=0，state=FAULT_LATCHED，reason通常BLOCKED
  → bench_cmp_fail.c:21 → Protection_RequestStop()
  → :22–25 → 保存一次快照，:30 completed=1（不是测试PASS）

main.c:55 → bench_cmp_fail_poll()
  → bench_cmp_fail.c:34 → Protection_Process()
  → 继续检查并保持锁存；没有注入、RUN或CLEAR命令
```

预期硬件链应是CMP1模拟输入→RAW→窗口/FAIL输入→FAIL1→MOE/六路默认态→CH0/1/2物理脚；当前程序未到达有效模拟输入/RAW，FAIL1也被禁用。不能把软件调用图画成一条已工作的完整硬件链。

当前源配置预期而非本轮寄存器实测：

| 项目 | 状态 |
|---|---|
| SYS_Module_MCPWM0 | bit10使能及读回代码已实现 |
| TCLK.CLK_EN | 1；CNT0_EN=0 |
| FAIL1_SEL | 1，CMP1来源已准备 |
| FAIL1_EN / MOE | 0 / 0 |
| CH_DEF低6位 / CH_MASK低6位 | 0x15 / 0x3F，HIN低/LIN高、六路响应默认态 |
| FLT / AUEN / TR0_AEC | 0 / 0 / 0；FLT=0不等于没有固定16周期滤波 |
| config_valid / raw_valid / hardware_qualified | 0 / 0 / 0 |
| RAW数值 | 占位0，不是测量到的健康状态 |
| RUN / MOE置1 / CLEAR | 平台BLOCKED；Manager在RAW无效时不会尝试ClearFail |
| P0～P4 | NOT VERIFIED（此前均NOT RUN） |

## 4. 问题清单（按严重程度）

### P0-01：真实比较器至FAIL1保护链尚未实现
**状态：NOT IMPLEMENTED；作为硬件测试/功率运行放行条件为FAIL。**

- 定位：firmware/platform/lks32mc071/lks_protection.c:54、65–69、116–125、127–130；SDK Source/lks32mc07x_mcpwm.c:170。
- Init仅配置MCPWM关闭态和FAIL1来源，没有CMP/DAC配置、真实RAW读取、有效模拟资格；StructInit清零后FAIL1_INPUT_EN未覆盖，故FAIL1_EN=0。SetMoe(1)恒BLOCKED，ClearFail没有EIF W1C操作。
- 影响：当前不可能完成真实比较器触发、异步三相关断和受控恢复；这不是一条“已实现但仅未实测”的链。
- 边界：禁用是刻意的安全阻断，本轮未发现错误地声称其已具备资格的生产代码；缺项不能通过改valid/enable标志消除。
- 推进：获准后补实际模拟配置/读取、明确FAIL极性/窗口和清除机制，并按阶段资格开放；不是重新增加Mock。

### P1-01：受保护逻辑PWM及注入/恢复测试入口未实现
**状态：NOT IMPLEMENTED。**

- 定位：firmware/tests/bench_cmp_fail/bench_cmp_fail.c:9–35；lks_protection.c:116–120；firmware/board/lks_evb_mvpowpre/board_init.c:31–40。
- Bench只有Init、STOP和一次快照；poll不接RUN/CLEAR/注入命令，没有有限时载波、故障激励、硬件先关的软件不干预观测段或完整P0～P4序列。
- 平台也未装载受保护测试PWM的周期/比较值、AF3复用和计数运行配置；GPIO保持AF0，不能把SetMoe接口存在当成已可输出测试PWM。
- 影响：即使日后只补CMP初始化和FAIL1_EN，仍不能直接执行既定测试。
- completed表示记录完成，四个结果保持BENCH_BLOCKED；没有当前误报PASS。init报告不会被poll动态刷新（bench_cmp_fail.h:20–21），后续真正注入时需可追溯的现场记录，当前不是一个隐藏“已运行”的测试器。

### P1-02：全局MOE开启权限不是唯一Manager入口
**状态：OPEN（架构/构建边界缺口）；当前CMP_FAIL分支错误开启旁路未证实。**

- 定位：firmware/platform/lks32mc071/bench_sixstep_output.c:102、139–148、162–168；firmware/tests/bench/test_gate_driver.c:27–30、86–95；A1/A2/A3各enable函数见下表。
- A4/A5通过后端直接置MOE，没有查Protection Manager锁存或真实平台资格；A1～A3通过SDK直接开启。SDK初始化还能重写FAIL配置并清EIF（mcpwm.c:134、170）。
- 这些是已存在的隔离台架接口，不擅自改成“当前CMP_FAIL运行已越权”。main.c:19–23的宏互斥与:53–95的分支选择防止当前CMP入口同时运行旧Bench；WP2 MAP确认旧enable路径被移除。
- 影响：Manager只管理经过它的请求，不能给全固件签“任何启动都经Manager”结论。未来若在受保护运行中混用旧Bench，可能重新开MOE或重置保护配置。
- 推进：先由主控明确哪些是台架例外，确保产品/保护构建不调用它们、固化入口与写权限规则；不要求本轮修改已验证A5，也不靠Manager临界区去约束绕过它的直接寄存器写入。

### P2-01：受版本管理入口与已验证本机入口不一致
**状态：FAIL（按HEAD直接重现CMP_FAIL入口）；本机主Target集成PASS复用WP2。**

- 定位：firmware/projects/keil/lks32mc071/LK_StdPeriph.uvprojx:342，HEAD=BENCH_TEST_GATE_DRIVER，工作树=BENCH_TEST_CMP_FAIL；main.c:53–60。
- 因此直接checkout远端main再构建默认Target，将走A5而不是上述Manager调用链；三个保护源文件在工程中存在不等于入口实际使用。
- 两份配置下三个保护文件都各加入一次；工作树条目:564–577、HEAD条目:567–580。问题不是遗漏源文件或重复编译。
- 推进：在后续授权配置管理时固定/记录主用保护入口；不能把未提交宏选择仅凭WP2成功就写成远端默认保护已集成。本轮不改工程，也不自行提交本机设置。

### P2-02：CMSIS依赖声明与真实构建版本不一致
**状态：OPEN。**

- 定位：同uvprojx:1165–1166声明CORE5.1.2/package5.4.0；WP2报告:16及其原始main.d确认实际用已安装CMSIS6.1.0。
- 已有编译PASS只对实际解析环境成立，不能声称验证了未安装5.4.0，也不能保证其他机器解析相同。
- 沿用WP2结果，不因此重构建、升级包或追究已弃用第二Target。后续固定依赖即可，不先消耗精力复核所有CMSIS内容。

### P3-01：阶段计划“当前状态”落后于已归档结果
**状态：OPEN。**

- 定位：tests/test_cases/V02-PLAN-000-workplan.md:3、13、17、19，仍称Mock未执行/NOT BUILT，并声明该段优先于历史文本。
- 与已提交的WP1/WP2报告不一致，容易让主控再次安排已完成验证。
- 以2026-11-18两份实际报告为准；后续允许编辑时更新入口状态即可，不另建审核流程。本轮只记录，不修改原文。

## 5. MOE写入与真实可达性核对

| 写入者 | 行号 | 当前CMP_FAIL入口是否调用 | 审核判断 |
|---|---|---|---|
| LksProtection_SetMoe | lks_protection.c:76–96 | 是，但仅关闭分支 | 置1恒BLOCKED；关闭先清MOE再停CNT0并读回 |
| SDK MCPWM_Init | SDK Source/lks32mc07x_mcpwm.c:117、127、134、170 | 经LksProtection_Init调用 | 装载关闭态；保存boot在清EIF之前；不是Manager外的开启请求 |
| A4/A5共享后端 | bench_sixstep_output.c:147 | 否 | 直接开启；WP2 MAP移除bench_gate_output_apply及相关未用代码 |
| A1/A2/A3 enable | test_mcpwm_single.c:171、test_mcpwm_complementary.c:174、test_mcpwm_three_pair.c:174 | 否 | SDK PWMOutputs(ENABLE)绕过Manager |
| SDK PWMOutputs | SDK Source/lks32mc07x_mcpwm.c:202–215 | 当前映像未保留 | 可同时置FAIL012/FAIL345 MOE；不能当产品授权入口 |
| HAL使能接口 | SDK hal/source/hal_lks32mc07x_mcpwm.c:423–427 | 未加入主Target | 库中存在不等于当前运行；不将整个SDK改造为新权限框架 |

未发现当前CMP_FAIL Bench直接置MOE=1；bench_cmp_fail.c:22直接调用平台Read仅为只读报告，不是操作权限旁路。

## 6. 已确认正确的实现（复用既有验证）

| 项目 | 结论及源码 |
|---|---|
| 安全GPIO初态 | PASS（静态）：main.c:51先board_init_safe；board_init.c:31–40预装H=0/N=1后切方向/复用；不是实测上电瞬态承诺 |
| 无效RAW不放行 | PASS：lks_protection.c:65–69明确无效，Manager:28–31先看有效性再判资格，FAULT_LATCHED保持关闭 |
| 失败初始化 | PASS：Manager:40–48不允许重Init清锁存；平台失败会latch并尝试关断，Bench:20–21记录结果且无RUN路径；不靠忽略返回值偷偷继续输出 |
| boot/首次故障 | PASS（WP1）：平台:111在SDK清EIF前保存；Manager:45保留历史故障，:16只在当前锁存期首次采样 |
| RUN/故障竞争 | PASS（WP1指定注入时点）：Manager:56–64先inspect、再申请、后inspect；新故障锁存。硬件瞬时重开安全仍NOT VERIFIED |
| STOP及源撤销 | PASS（WP1）：Manager:75–78先检查故障、再关闭；:105–107普通Process不清锁存、不自动重启；STOP不是CLEAR |
| CLEAR | PASS（WP1假设平台合格）：Manager:87–95检查源有效撤销/MOE0/锁存态，平台清后复查，成功只DISARMED；真实ClearFail仍NOT IMPLEMENTED |
| 关断失败 | PASS（软件报告语义）：Manager:15–20保留锁存并优先返回关断错误；平台:94–96读回，不虚报已经关闭 |
| 嵌套临界区 | PASS（WP1模型+源码）：平台:22–28保存PRIMASK并恢复原值；不屏蔽异步硬件FAIL |
| CMP1路由与默认态 | PASS（静态选源/关闭态）：平台:118选1，SDK:170移到FAIL1_SEL[11:10]；默认助手:4–10设置六路mask/P低N高/AEC0。FAIL1输入仍禁用 |
| MCPWM时钟位 | PASS（复用专项）：平台:106、SDK:113使用SYS_Module_MCPWM0 bit10；未发现把TIMER0 bit4当MCPWM使能的新错误 |

当前未发现新的Manager错误清锁存/自动重启缺陷，不重复证明WP1；其证明范围仍是指定接口模型，非绝对竞态安全。

## 7. 尚未实现与必须实机验证的项目

尚未实现（NOT IMPLEMENTED）：
- 可核实量程/共模/稳定时间的CMP1/DAC/REF配置及真实RAW读取。
- 获资格的FAIL1_EN/极性/窗口/滤波组合、受保护有限PWM输出。
- 仅清FAIL1事件的EIF W1C及清后读回，不自动开MOE。
- Bench真实单次激励、故障先关而软件延后补救的观测、保持/清除/显式再RUN及有效记录。
- 真实六扇区电流感知/保护覆盖与CBC安全周期恢复；当前锁存Manager不是最终CBC实现。

已有文档/硬件条件不再重审，以下保留OPEN或NOT VERIFIED：
- DAC/REF电压范围、比较器共模/极性、CMP RAW翻转；SDK函数存在不是模拟资格。
- 关闭窗口时的真实通路与极性关系；FCLK入口编号和图示分频差异沿用时钟报告OPEN。模块bit10已确认，不把它重新列为未知。
- 硬件故障发生时六个pad确实H=0/N=1、mask/默认态有效、滤波宽度及硬件先关的可靠时间参考。
- 持续FAIL与软件写MOE的优先关系、瞬时重开窗口、故障保持/撤销/显式清除；PRIMASK和Mock都不能替代。
- 计数器/更新事件与未来CBC本周期截断、下一安全边界恢复，严重锁存优先；不得直接AEC=1当作CBC完成。
- 六路MCU输出到未供电LKS560的物理隔离及安全控制板供电证据。本轮未接触硬件，不能把旧A5“P3摘掉/母线0V”当成已经断开六路输入。

这些项目按原P0～P4顺序处理；CMP_RAW准备不必等待所有CBC细节都实测，但任何实际执行须由主控按对应门槛授权。此次不新拆工作包、不制定新的通电指令。

## 8. 是否具备下一阶段条件

| 决策 | 结论 |
|---|---|
| WP1已完成的软件状态机覆盖 | PASS，复用原记录 |
| WP2主用Target编译链接 | PASS，仅实际本机配置及已记录依赖；不重建 |
| 当前CMP_FAIL程序不放行未获资格输出 | PASS，静态路径限定 |
| 完整硬件保护功能已实现 | NOT IMPLEMENTED |
| 固件全局唯一MOE授权 | OPEN，旧Bench例外须隔离 |
| 直接执行P0～P4或功率试验 | FAIL（当前条件不足），实机链NOT VERIFIED |
| 主控据本报告授权下一步补实现/确认前置 | 可以；本轮不自行开始 |

建议主控只优先处理：固定主用保护入口与MOE权限边界，补齐真实CMP/RAW及受保护Bench缺项，再按既定物理/模拟前置条件决定实测放行。不要再把已完成Mock和Keil构建作为下一项重复工作。

本轮结束，等待主控审核及后续授权。本文不授予修改代码、启用保护门禁或操作硬件的权限。
