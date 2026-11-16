# V02-01 平台保护接口与锁存状态机设计

- 日期：2026-11-16
- 基线：GitHub main / 550897aa22481a899a0a09b399052991570599ec
- 状态：**DESIGN PREPARED / NOT IMPLEMENTED / NOT BUILT / NOT RUN**
- 范围：只定义平台接口、寄存器目标值、初始化顺序、软件故障状态机和隔离逻辑测试门槛；不修改固件，不构建、不烧录、不执行硬件动作。
- 上位依据：[V02-00静态审计](../reports/2026-11-16_V02-00-static-audit.md)、[V0.2总计划](V02-PLAN-000-workplan.md)、[内部保护链P0–P4计划](V02-PROT-001-internal-path-plan.md)。

## 1. 设计目标与边界

V02-01首先用**锁存模式**证明CMP1原始比较结果能够经MCPWM FAIL1硬件关闭CH0/1/2，并且六路MCU逻辑输出进入LKS560要求的安全默认态。此阶段不是CBC验收，也不改变最终目标：常规过流以CBC逐波限流为主，严重/持续故障以更高优先级锁存为辅。

本设计必须保持以下边界：

1. CMP0继续保留给BEMF；本阶段只使用CMP1候选输入，不挪用CMP0。
2. CMP1输入候选为DAC0/REF内部比较，用于逻辑链路验证，不代表真实Shunt/OPA电流链已验证。
3. 首轮关闭CMP开窗，避免把PWM窗口门控混入基本FAIL路由；但“所有窗口位为0时的直通语义”仍须在P1前确认，不能仅凭推断放行。
4. TR0_AEC保持关闭；本阶段禁止自动逐周期恢复。
5. 所有P0–P4都要求控制板六路MCU输出与未供电LKS560 HIN/LIN物理断开，并保留断电通断证据。
6. A0–A5入口与参数保持不变；新增Bench入口必须与现有入口互斥。当前main.c尚无BENCH_TEST_CMP_FAIL入口。
7. 本文中的函数名是接口职责示意，不是已提交的C API。

## 2. 软件分层与唯一MOE控制权

建议边界：

| 层 | 职责 | 禁止事项 |
|---|---|---|
| Platform / LKS32MC071 | CMP/DAC/MCPWM寄存器配置、读回快照、FAIL事件W1C清除、MOE和CNT0控制、写保护封装 | 不决定电机是否允许RUN，不实现CBC策略 |
| Board / LKS_EVB_MVPOWPRE | HIN/LIN引脚映射、CH_DEF安全态、CMP输入MUX候选、DAC/REF量程配置、FAIL极性配置 | 不直接操作运行状态机 |
| Application / Protection Manager | BOOT、DISARMED、RUNNING、FAULT_LATCHED状态；RUN/STOP/CLEAR权限；故障记录与优先级 | 不散布MCU寄存器魔数 |
| Tests / Bench | 单次CMP注入、有限PWM观察、状态快照与测试归档 | 不复制A5六步输出后端，不绕过Protection Manager写MOE |

概念接口职责：

- Platform：安全初始化、读取硬件快照、配置/使能FAIL1、清除FAIL1事件、设置MOE、启动/停止时基0。
- Application：初始化Protection Manager、请求RUN、请求STOP、请求CLEAR、周期性检查硬件状态、读取软件状态。
- Bench：只调用Application层接口并执行有限测试序列。

**唯一MOE写入规则：** V02-01测试入口及未来产品运行路径中，只有Protection Manager可以请求MOE置1。A5当前bench_sixstep_output.c存在直接置MOE路径，PWMOutputs()也会同时操作FAIL012/FAIL345；V02-01必须使用独立互斥入口，不调用这些A5输出函数。产品化前还要审查所有MOE写入点，不能只靠软件约定。

## 3. 状态机

状态定义：

| 状态 | 硬件要求 | 允许动作 |
|---|---|---|
| BOOT_SAFE | MOE=0；GPIO先保持HIN=0/LIN=1；不自动RUN | 捕获启动前可读故障标志，进入配置 |
| CONFIGURING | MOE=0；CNT0不运行；FAIL1输入暂不使能 | 配置CMP/DAC/FAIL/默认态并读回 |
| DISARMED | MOE=0；无活动故障；配置读回正确 | 接受一次显式RUN或CLEAR以外的STOP |
| RUNNING | 显式RUN后才允许MOE=1；FAIL1硬件路径持续有效 | 正常有限Bench输出；监视EIF/RAW/MOE |
| FAULT_LATCHED | MOE=0；保持CH_DEF；软件锁存不可自动清除 | 只接受故障检查与显式CLEAR请求 |
| CLEAR_CHECK | MOE=0；RUN请求撤销；比较器输入必须已回到无故障态 | W1C清事件并读回；通过后回DISARMED |

状态转换：

- Reset → BOOT_SAFE：不自动恢复输出；启动后必须由上层显式请求RUN。
- BOOT_SAFE → CONFIGURING：先保存可读的启动故障快照，再初始化外设。注意SDK的MCPWM_Init()会向MCPWM0_EIF写0xFFFF清标志，调用前必须先保存原标志，不能把初始化清标志误认为故障不存在。
- CONFIGURING → DISARMED：仅当CMP1_RAW处于确认的无故障态、FAIL1_IF清零、MOE为0、默认态/mask/时钟/选择寄存器读回符合目标值时允许。
- CONFIGURING → FAULT_LATCHED/BLOCKED：任何输入极性不明、时钟不明、寄存器读回不符或故障活动，均保持输出关闭。
- DISARMED → RUNNING：只接受显式RUN；RUN前检查RAW无故障、FAIL1_IF=0、软件无锁存、配置有效。活动故障或标志未清时拒绝RUN。
- RUNNING → FAULT_LATCHED：FAIL1_IF置位、CMP1_RAW进入故障态、MOE意外变0或保护配置丢失时立即进入软件锁存。硬件FAIL应先完成输出关断，软件只负责记录并阻止后续重开。
- RUNNING → DISARMED：显式STOP，先清MOE，再停止CNT0；STOP不能清除故障锁存。
- FAULT_LATCHED → CLEAR_CHECK：必须有显式CLEAR请求，且RUN请求已撤销、MOE=0、故障源已撤销并稳定。
- CLEAR_CHECK → DISARMED：仅在按手册W1C清除FAIL1_IF后，读回标志仍为0、RAW仍为无故障态且MOE保持0时允许。
- CLEAR_CHECK失败：任何标志重置/重新置位、RAW仍活动或状态不一致，返回FAULT_LATCHED。
- CLEAR成功**不置MOE**；重新输出必须另行显式RUN。

优先级：硬件FAIL异步关断 > 软件FAULT_LATCHED > 显式STOP >（未来CBC逐周期禁止）> RUN请求。CBC状态不在V02-01实现。严重故障锁存期间，任何周期更新或普通RUN都不得自动恢复。

**复位边界：** 当前设计只定义本次启动周期内的软件锁存；软件RAM锁存不会天然跨MCU复位保存。复位后仍必须默认MOE关闭且不自动RUN。是否要求严重故障原因跨复位持久化，留到产品故障策略评审，不在本轮假定已有NVM/备份寄存器实现。

**RUN竞态未决：** 必须验证FAIL源持续有效或EIF已置位时，硬件是否持续压制MOE，以及软件清标志/置MOE顺序是否存在重开窗口。在该行为未实测前，不得宣称仅靠软件检查即可形成量产安全重启机制；P3用于暴露此问题，未通过则RUN放行保持BLOCKED。

## 4. 寄存器预期配置表

下表是V02-01的**目标配置/读回清单**，不是已经写入固件的配置。FAIL极性、DAC数值、参考电压和滤波时钟仍需确认。

| 寄存器/字段 | 目标值或策略 | 读回/验收 | 依据与状态 |
|---|---|---|---|
| SYS_CLK_FEN MCPWM0 | 模块门控bit10开启 | 读回bit10 | 仓库系统时钟表/SDK一致；MCPWM_Init会使能模块 |
| SYS_CLK_FEN CMP数字模块 | bit2开启（用于CMP数字寄存器访问/RAW读取接口） | 读回bit2 | SDK SYS_Module_CMP定义；不等于模拟CMP1使能 |
| SYS_AFE_REG5 CMP1 | bit7开启 | 读回bit7 | CMP1模拟模块使能；SDK SYS_AnalogModule_CMP1 |
| SYS_AFE_REG5 DAC0 | bit12开启 | 读回bit12 | DAC0模拟模块使能；候选注入用 |
| SYS_AFE_REG3 CMP1 MUX | P端[14:12]=7（DAC0）；N端[5:4]=1（REF） | 读回MUX字段 | SDK列出CMP1_SELP_DAC0=7、CMP_SELN_REF=1；REF范围/共模待核实 |
| DAC0 | 选择已核实量程，加载校准值，内部寄存器输出两档 | 记录量程、DAC码、REF与RAW对应关系 | DAC_Init加载trim；DAC_OutputValue写内部DAC；不预设固定电压 |
| DAC0外部引脚输出 | 不使能DACOUT_EN，不把DAC0接到外部P0.0 | 确认外部输出未路由 | 本测试只使用内部CMP MUX |
| CMP_CFG CMP1_IN_EN | bit5=1 | 读回bit5 | CMP1输入使能 |
| CMP_CFG CMP1_POL | 先按P0观察结果确定 | 记录配置与RAW真值表 | bit4：0高有效、1低有效；不得只凭P/N推测 |
| CMP_CFG CMP1_W_PWM_POL | 初始不作为消隐配置 | 与BLCWIN一起记录 | bit7仅在开窗使能时参与 |
| CMP_BLCWIN CMP1窗口位 | 初始全部为0 | 读回[7:4]=0 | 期望关闭窗口；直通语义仍是P1前置核实项 |
| CMP_DATA CMP1_RAW_DATA | 读取bit1 | DAC两档下记录0/1 | 手册表10-7；CMP_GetValue()返回RAW三通道位 |
| MCPWM0_TCLK CLK_EN | bit2=1 | 读回bit2 | MCPWM工作时钟使能 |
| MCPWM0_TCLK CLK_DIV | 初始编码0 | 读回[1:0]=0 | MCPWM时基时钟不分频；不是SYS_CLK_FEN门控 |
| MCPWM0_TCLK BASE_CNT0_EN | 配置时0；有限PWM测试开始时才置1 | 读回bit6 | 计数器开关；与FAIL滤波时钟门控不是同一字段 |
| MCPWM0_FLT FLT_CLKDIV | 初始0（最小二级分频） | 读回[7:0]=0 | 仍是固定16周期滤波，不是关闭滤波 |
| MCPWM0_FAIL012 FAIL1_SEL | [11:10]=01，选择CMP1原始输出 | 读回字段=1 | 手册表14-48；SDK结构体注释有误，按位表 |
| MCPWM0_FAIL012 FAIL1_EN | bit5=1；CMP稳定后才使能 | 读回bit5 | 使能FAIL1输入 |
| MCPWM0_FAIL012 FAIL1_POL | 根据P0实测RAW有效态选择 | 记录真值表与bit3 | 0高有效、1低有效；与CMP1_POL是不同配置层 |
| MCPWM0_FAIL012 FAIL0_EN | bit4=0 | 读回bit4 | 保留CMP0/BEMF分工 |
| MCPWM0_FAIL012 MOE | 配置、故障、清故障期间均为0；仅显式RUN可置1 | 读回bit6 | FAIL事件会硬件清MOE；软件不能在锁存态重写 |
| MCPWM0_CH_DEF | 低6位=0x15 | 读回[5:0]=0x15 | CH0/1/2的N默认高、P默认低 |
| MCPWM0_CH_MASK | 低6位=0x3F | 读回[5:0]=0x3F | 六个P/N输出均响应MOE/FAIL默认态 |
| MCPWM0_SDCFG TR0_AEC | bit6=0 | 读回bit6=0 | 禁止更新事件自动清EIF并恢复MOE |
| MCPWM0_SDCFG TR0_T0/T1_UEN | 初始均0 | 读回bit4/5=0 | 首轮不启用自动更新恢复 |
| MCPWM0_AUEN | 初始0 | 读回相关位 | 不让自动更新机制混入首轮锁存测试 |
| MCPWM0_EIE FAIL1_IE | P1硬件独立证明时先为0 | 读回bit5=0 | 先不让ISR干预；事件标志与中断使能分开 |
| MCPWM0_EIF FAIL1_IF | 事件发生时bit5置1；仅源撤销后按W1C清 | 写BIT5后读回0且不重置 | 手册表14-42：RW1C；不要用读改写清零 |
| MCPWM0_PRT | 平台封装DEAD解锁、CAFE重新锁定 | 读回受保护寄存器，不散布魔数 | FAIL/CH_DEF/CH_MASK/SDCFG/TCLK等受写保护 |

### 关键配置解释

- 当前A5的lks560_pwm_set_off_defaults()已经把CH0/1/2六路设置为P低/N高，并打开六路FAIL响应；对应低6位CH_DEF=0x15、CH_MASK=0x3F。该配置不等于CMP1/FAIL1已启用。
- 当前bench_sixstep_output_init()设置TR0_AEC=DISABLE、T0自动更新关闭、AUEN=0；但apply_requests()会直接置MOE。V02-01不能复用该A5运行函数。
- SDK MCPWM_Init()会写MCPWM0_EIF=0xFFFF清除标志，且按结构体字段重写FAIL寄存器；调用前先保存旧EIF，初始化后确认MOE=0、FAIL1尚未使能。
- SDK CMP_Init()对SYS_AFE_REG3输入MUX字段使用OR写入而非先清字段；如果不是上电复位默认值，必须先按掩码清理CMP1_SELP/SELN字段，避免旧配置残留。
- CMP1_RAW_DATA绕过CMP数字接口滤波，但CMP1_POL、CMP1_W_PWM_POL、CMP_BLCWIN及MCPWM FAIL滤波是不同处理层；P0先记录实际RAW真值表，P1再冻结FAIL1_POL。
- MCPWM0_EIF的FAIL1_IF是写1清零；清除动作不应顺带置MOE。TR0_AEC必须保持0。
- FAIL滤波名义宽度仍是16周期。当前FLT=0、CLK_DIV=0在MCLK=48MHz假设下名义约0.333µs；这不是总响应延迟实测。

## 5. 初始化与故障处理顺序

1. **安全GPIO先行：** board_init_safe()建立HIN=0/LIN=1。此时不得连接未供电LKS560输入。
2. **记录旧状态：** 在任何会清EIF的MCPWM_Init()前读取并保存MCPWM0_EIF、FAIL012、CH_DEF、CH_MASK、TCLK、FLT、SDCFG。
3. **MCPWM安全初始化：** MOE保持0、BASE_CNT0_EN=0、FAIL1输入暂不使能、TR0_AEC=0、T0/T1自动更新关闭、AUEN=0；写入CH_DEF=0x15、CH_MASK低6位=0x3F。
4. **时钟审计：** 分别确认SYS_CLK_FEN MCPWM0模块门控、MCPWM0_TCLK.CLK_EN、CMP数字模块门控、CMP1/DAC0模拟使能。不得因手册FCLK[4]冲突而擅自打开SYS_CLK_FEN bit4（该位在SDK/系统时钟表中分配给TIMER0）。
5. **配置CMP/DAC：** 按已核实的MUX字段配置DAC0/REF候选，CMP1输入使能，关闭所有CMP1窗口位；DAC0只供内部比较器，不使能外部DAC引脚。
6. **P0确认RAW：** MOE=0、CNT0停止、FAIL1_EN=0；用两档安全DAC值记录CMP1_RAW，确认比较方向和稳定性。若REF范围、共模或DAC量程不明，停在P0，不猜电压。
7. **配置FAIL1但不RUN：** 先设置FAIL1_SEL=1和已测得的FAIL1_POL，保持FAIL1_EN=0；确认CMP1_RAW处于无故障态后，按W1C清除FAIL1_IF并读回。
8. **使能FAIL1：** 仅在FAIL时钟来源、窗口关闭语义和极性均有证据后使能FAIL1；读回SEL/EN/POL、滤波、CH_DEF、CH_MASK、AEC和MOE。若一使能就产生FAIL，保持MOE=0并查因。
9. **有限逻辑RUN：** 只有P1放行后，Bench才启动CNT0和有限PWM；FAIL1_IE先关闭，保留CPU不写MOE/STOP/GPIO的短观察段，确认硬件先关断后再执行软件兜底。
10. **锁存：** 观察到FAIL1_IF或运行中MOE意外清零，软件转FAULT_LATCHED；禁止调用A5输出后端，禁止自动恢复。
11. **清故障：** 确认RAW已撤销并稳定、RUN请求撤销、MOE=0；只向EIF写FAIL1_IF对应BIT5清除，读回仍为0；若重新置位则继续锁存。
12. **显式RUN：** CLEAR只把状态返回DISARMED，不置MOE；用户/上层必须另行请求RUN。

## 6. P0–P4隔离测试设计与放行门

### 所有阶段共同前置条件

- 控制板、功率板、LKS560、J-Link和注入源全部断电后，物理分离控制板与功率板。
- 用万用表确认P1.4～P1.9六路MCU输出到LKS560 HIN/LIN均断开；排除排线、飞线、探头附件或其他旁路，保存断电通断记录和连接照片。
- 主母线物理断开并确认储能释放；功率板、LKS560、功率级不供电；无电机。
- 仅在另行接线审核后给控制板供电；确认控制板5V来源，避免J-Link与板载5V双源供电。
- 不在带电状态插拔两板；不通过短接母线/相线制造过流。
- 物理隔离是人工检查项，软件无法自行证明。未取得六路断开证据，所有P0–P4均BLOCKED。

| 阶段 | 操作 | 预期结果 | 失败/阻塞判据 |
|---|---|---|---|
| P0 CMP RAW | PWM关闭、MOE=0、FAIL1_EN=0；经确认的DAC0/REF候选输入切换两档，读CMP1_RAW_DATA bit1；记录CMP_CFG/AFE/DAC值 | RAW可重复地出现两个逻辑状态，比较方向明确；没有PWM输出 | REF范围、DAC量程/共模或RAW不翻转：停止，不开PWM |
| P1 硬件FAIL | 关闭窗口并确认直通语义；解决FCLK门控冲突；使能FAIL1，AEC=0，FAIL1中断先关闭；有限逻辑PWM下单次注入 | CPU未先写STOP/MOE时，硬件置EIF bit5、清MOE，六路pad进入P/N=0/1 | FCLK来源或窗口语义未解决则BLOCKED；若必须软件先关才安全则FAIL |
| P2 六路默认态 | 分别对U/V/W运行受控逻辑状态并触发FAIL；每次两探头测同一相P/N | U/V/W每相均为HIN=0、LIN=1；CH_DEF/mask与物理pad一致 | 任一脚不是安全默认态，或只看到MOE=0但pad不符，停止 |
| P3 锁存与恢复 | 故障有效时请求RUN；撤销输入但不清EIF再次请求RUN；显式CLEAR；确认MOE仍0；再单独显式RUN | 活动故障/未清标志/软件锁存均拒绝RUN；CLEAR不自动输出；新RUN必须独立命令 | 活动FAIL下硬件可被意外重新置MOE、标志无法稳定清除或状态不同步：保持BLOCKED |
| P4 重复与归档 | 仍保持六路物理隔离，重复P0–P3、测试超时/复位默认态和故障边界；建议先做10次重复（次数为初始建议，执行前冻结） | 逻辑结果可重复，任何故障均不自动重启，证据与固件指纹齐全 | 任何偶发错误默认态、误重开或配置漂移，退回定位，不升级到Gate/功率级 |

### P1当前明确阻塞

1. V02-00发现MCPWM滤波说明中FCLK[4]与系统时钟表/SDK门控位冲突：系统表/SDK将SYS_CLK_FEN bit4分给TIMER0、bit10分给MCPWM0。不得自行开启bit4试错。
2. CMP章节转换文本已删除原图；CMP_BLCWIN全部关闭时FAIL是否无延迟直通，以及开窗抽头/组合逻辑仍须核实。
3. 16周期滤波的边界相位和总响应延迟没有实测；0.333µs只是48MHz假设下的名义滤波宽度。
4. TR0_AEC与持续FAIL优先级、手动UPDATE行为尚未验证；V02-01保持AEC=0，不测试自动周期恢复。

P0在物理隔离、DAC0/REF范围与共模确认后可以单独评估；P1未关闭以上阻塞前不进入。P2–P4依赖P1的硬件路径通过。所有步骤仍然是计划，当前全部NOT RUN。

## 7. 验收证据与记录模板

每次只记录实际观测，不以寄存器配置代替波形，不以软件STOP代替硬件FAIL证明。

| 字段 | 记录内容 |
|---|---|
| 软件 | Git commit、编译器/Keil版本、编译宏、HEX SHA、板上固件身份 |
| 硬件 | MCU/功率板版本、LKS560型号/批次、连接器状态、物理隔离通断证据 |
| 供电 | 控制板供电来源/电压；驱动器、功率级、主母线均确认无供电 |
| CMP/DAC | CMP1 MUX、CMP1_POL、窗口配置、DAC量程/码值、REF测量或依据、RAW读数 |
| MCPWM | FAIL012、EIF、EIE、CH_DEF、CH_MASK、TCLK、FLT、SDCFG、CNT0 |
| 波形 | 目标MCU pad、CMP/故障参考、采样条件、时间基准、截图/文件哈希 |
| 结果 | P阶段、PASS/FAIL/BLOCKED、失败现象、是否软件写过MOE、下一步 |

## 8. 当前结论

- **已完成：** V02-01平台边界、寄存器目标值、初始化顺序、RUN/STOP/CLEAR/FAULT_LATCHED状态机及P0–P4放行设计。
- **未完成：** 源代码实现、独立Bench入口、宿主测试、Keil构建、HEX指纹、任何硬件逻辑验证。
- **仍开放：** FCLK门控编号、CMP窗口关闭语义、CMP1/FAIL1极性组合、持续FAIL下MOE优先级、手动UPDATE/AEC行为、DAC0/REF实际量程与共模。
- **禁止升级：** 未取得物理隔离证据和P1逻辑FAIL通过前，不给LKS560/功率级供电；未关闭A5安全遗留及完整保护验收前，不接主母线或电机。
- **最终架构不变：** CBC逐波限流为常规一级，严重/持续故障锁存为二级；本轮锁存Bench仅验证内部硬件关断链，不代表CBC实现。
