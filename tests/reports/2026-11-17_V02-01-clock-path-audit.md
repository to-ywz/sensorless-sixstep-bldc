# V02-01 MCPWM FAIL 滤波时钟专项审计

- 日期：2026-11-17；MCU：LKS32MC071CBT8。
- 基线：main / `2049c6e7be4b673873adf4d21b75a75998633029`。
- 状态：**CLOCK PATH STATIC AUDIT DOCUMENTED / V02-01 IMPLEMENTED PARTIAL / NOT BUILT / NOT HARDWARE TESTED**。
- 本次专项审计执行本身为只读；“CODE UNCHANGED”仅描述审计操作未修改固件，不表示同一Git提交仅包含Markdown。
- 范围：模块门控、FAIL内部采样时钟和FCLK编号差异；原版页面已渲染目视检查，非仅文字提取。审计过程未构建、烧录或执行硬件动作；代码与文档合并提交边界见第9节。
- 原始手册：`LKS32MC07x_User_Manual_v1.69.pdf`，424页；本地来源为用户Downloads目录，不将厂商原件上传仓库。
- 手册SHA256：`41369D4CC7CF3314B9E312391DBDD9A2D139E48D99BC89D7ACB47E03E8B2B29E`。
- 标签：MANUAL=原版直接证据；SDK=代码/宏；OFFICIAL=官方资料；INFERENCE=推断；OPEN=不能确认。

## 1. 模块门控：已确认

| PDF页序 | 印刷页 | MANUAL证据 |
|---|---|---|
| 73 | 29 | 图6-1：FCLK[10]接MCPWM0，FCLK[7:4]接TIMER3/2/1/0 |
| 81–82 | 37–38 | 表6-5：SYS_CLK_FEN地址0x4000009C，bit10 MCPWM0、bit4 TIMER0 |

**SYS_CLK_FEN bit10是MCPWM0模块时钟门控，已确认（MANUAL + SDK）。bit4属于TIMER0，不得用作MCPWM0模块使能。**

SDK定位均相对`firmware/projects/keil/lks32mc071/lks32mc07x_periph_driver/`：

- `Include/lks32mc07x_sys.h:146`：SYS_Module_MCPWM0=BIT10。
- `Include/lks32mc07x_user_manual.h:228`：MCPWM0门控宏为BIT10。
- `Source/lks32mc07x_sys.c:184`：SYS_ModuleClockCmd解锁、SYS->CLK_FEN读改写、重新锁定。
- `Source/lks32mc07x_mcpwm.c:113`：MCPWM_Init实际调用模块时钟使能。

调用链（SDK）：LksProtection_Init → SYS_ModuleClockCmd(SYS_Module_MCPWM0, ENABLE) → CLK_FEN置bit10；随后MCPWM_Init再次置同一bit10。不是只依据宏名称推定驱动行为。

## 2. 内部滤波链：图示已确认，入口编号仍OPEN

| PDF页序 | 印刷页 | MANUAL证据 |
|---|---|---|
| 240 | 196 | 图14-3：计数器使用TCLK，并由BASE_CNTx_EN控制 |
| 241 | 197 | 图14-5下方正文确实印刷MCLK/FCLK[4] |
| 242 | 198 | 图14-6：滤波时钟生成逻辑，入口仅标FCLK，无编号 |
| 266–267 | 222–223 | 表14-33：FLT地址0x40010C70、[7:0]分频字段 |
| 269–270 | 225–226 | 表14-36：TCLK地址0x40010C7C及控制字段 |

图14-6直接支持（MANUAL）：

```text
FCLK（未编号）
  → MCPWM0_TCLK.CLK_EN[2]
  → MCPWM0_TCLK.CLK_DIV[1:0]
  → TCLK
  → MCPWM0_FLT.FLT_CLKDIV[7:0]
  → filter clock
  → 16周期滤波器

TCLK → BASE_CNT0/1_EN[6/7] → 对应时基计数器（图14-3）
```

| 层级 | 输入→输出 | 控制 | 使能/分频与证据 |
|---|---|---|---|
| 系统 | 系统时钟源→MCLK | SYS_CLK_CFG | 系统选择/分频，图6-1（MANUAL） |
| 模块 | MCLK→FCLK[10]→MCPWM0 | SYS_CLK_FEN[10] | 独立模块门控，图6-1/表6-5及SDK |
| 工作时钟 | 图14-6 FCLK→工作时钟路径 | TCLK[2] | 共用CLK_EN，不是滤波器专属源选择（MANUAL） |
| 一级分频 | 工作时钟→TCLK | TCLK[1:0] | 1/2/4/8分频，无源选择字段（MANUAL + SDK） |
| 计数支路 | TCLK→CNT0/1 | TCLK[6/7] | 计数器独立使能，不能当作滤波时钟使能（MANUAL） |
| 二级分频 | TCLK→filter clock | FLT[7:0] | 正文/位表支持1～256；未发现独立门控字段（MANUAL + SDK） |
| 滤波判定 | filter clock→稳定输入判定 | 固定16周期 | FAIL章节及FLT说明（MANUAL） |

FCLK[10]与图14-6入口为同一网络是较强INFERENCE，仍OPEN；不能把推断升级为原版已明确编号的事实。模块时钟、计数器时钟、滤波采样时钟不是同一概念；后两者具有不同控制和分频。停止CNT是否不影响滤波为图示支持的预期，尚非实测。

名义模型：`T_filter = 16 × 2^CLK_DIV × (FLT_CLKDIV + 1) / f_MCLK`。

这是已有时钟模型、时钟有效条件下的名义滤波宽度；不代表比较器传播、滤波相位、MCPWM响应与IO传播组成的总保护延迟。CLK_DIV=0为1分频，FLT_CLKDIV=0也不关闭固定16周期滤波。上游网络编号仍待厂商确认。

## 3. MANUAL INCONSISTENCY / OPEN

1. 图14-6二级分频方框印为1/1～16，正文与8位字段定义支持1～256。
2. 图14-3将CLK_EN画在一级分频后，图14-6画在前。不能据简图确定硅片中门控的具体物理位置。
3. 图14-5下方FCLK[4]为原版印刷内容，不是OCR错误。
4. 已检查页面未见解释该编号的脚注。

保留差异，不擅自择取某图或某段正文作为最终硅片事实。分频目标沿用既有位表模型，不意味着上述差异已获厂商更正。

## 4. SDK执行代码与注释

| 位置 | SDK证据 |
|---|---|
| Source/lks32mc07x_mcpwm.c:117 | MCLK_EN写TCLK bit2，CLK_DIV写[1:0] |
| 同文件:145 | IO_CMP_FLT_CLKDIV写FLT |
| 同文件:380–395 | 工作时钟及一级分频接口操作TCLK |
| 同文件:597起 | 模块时钟接口操作SYS门控 |
| hal/source/hal_lks32mc07x_mcpwm.c:193、233 | 注释写SYS_CLK_FEN[4] |
| 对应:194、234 | 执行代码使用SYS_CLK_FEN_MCPWM0_CLK_EN_BIT，实际BIT10 |
| 同HAL文件:832 | SetFailFilter只写FLT，不选择时钟源 |

已检查标准库、HAL和寄存器定义，未发现专属于FAIL滤波器的源选择或独立门控；CLK_EN是MCPWM工作时钟使能。HAL注释与执行代码不一致，加强历史编号/文档错误的怀疑（INFERENCE），不能证明芯片内部物理连线。

差异对照：系统图/位表与实际SDK均用bit10；FAIL正文及HAL注释出现4；图14-6入口无编号。FLT只有分频配置，不能从接口缺失反证芯片绝无未公开时钟控制。

## 5. 官方勘误和版本资料

- OFFICIAL：[官方产品目录](https://www.lksmcu.com/index.php/LKS07Series/)列出中文UM V1.69及Errata V1.2。
- OFFICIAL：[官方Errata V1.2](https://sjk.lksmcu.com/static/upload/file/20230113/LKS32MC07x_Errata_v1.2.pdf)已检查，涉及ADC空闲采样芯片版本差异、分频条件下看门狗；未见MCPWM FCLK编号解释。
- OPEN：旧版V1.30官方链接返回404，未完成原文对比；[英文手册目录](https://www.lksmcu.com/index.php/UserManual-en/)所列V1.68文件获取超时；未确认实物芯片修订版本。
- SDK：仓库version.txt含MCPWM修正记录，未找到明确FCLK编号修正条目；仓库转换手册也有FCLK[4]，不是独立原版确认。
- OFFICIAL/OPEN：[官方驱动目录](https://www.lksmcu.com/index.php/Reference_2/)列3.16，但未将官网包与仓库逐文件认证。

官方已公开勘误未提及本问题，不等于不存在其他勘误或版本差异。

## 6. 五项结论

| 问题 | 结论与来源 |
|---|---|
| MCPWM0模块门控 | BIT10，已确认（MANUAL + SDK） |
| MCPWM FAIL滤波器内部时钟链路 | 图14-6链路已确认；指手册图示，不是实测（MANUAL） |
| FCLK[4]是否笔误 | 高度疑似，但证据不足（INFERENCE / OPEN） |
| FCLK[10]是否就是滤波器上游FCLK | 较强推断，仍OPEN；缺厂商直接确认 |
| 是否存在独立滤波时钟源选择/门控 | SDK和已检查寄存器中未发现（SDK + MANUAL范围内） |

bit10是上游模块门控，最终采样时钟还受内部工作时钟使能及分频影响。同一派生时钟体系是较强推断，停钟行为和跨修订一致性仍OPEN。

## 7. 当前固件审查及后续建议

本专项未发现需要立即修正的MCPWM时钟配置错误（SDK）：LksProtection_Init先置bit10再访问MCPWM；SDK再置一次为重复使能，不是切换时钟；MCLK_EN=1、CLK_DIV=0、FLT_CLKDIV=0符合目标；未发现错误使用bit4或错误滤波时间计算代码。

### 7.1 当前源码已实现项（静态源码核对；未运行验证）

| 项目 | 源码状态 | 依据与边界 |
|---|---|---|
| SYS_CLK_FEN快照 | IMPLEMENTED | `LksProtection_Read()`将SYS_CLK_FEN写入快照字段`sys_clk_fen`；未进行运行时验证。 |
| MCPWM0 bit10使能与读回 | IMPLEMENTED | `LksProtection_Init()`调用SDK使能MCPWM0并读回bit10；`LksProtection_SetMoe(0)`在初始化前也先使能并核对bit10。只证明代码路径，不证明滤波器入口网络编号。 |
| 初始化前访问行为 | IMPLEMENTED PARTIAL | `Read()`在未初始化或门控关闭时不访问MCPWM/CMP；`SetMoe(0)`允许先建立模块时钟后执行安全关断；`SetMoe(1)`仍返回BLOCKED。未运行验证。 |
| 关闭态配置与硬件资格分离 | IMPLEMENTED | `safe_config_matches`只检查关闭态配置子集；`hardware_qualified`与`config_valid`仍为0。 |
| CMP_RAW有效性保护 | IMPLEMENTED | `raw_valid`恒为0，`raw=0`仅是占位；当前没有CMP/DAC初始化，也不把占位值当作无故障证据。 |

### 7.2 仍未实现或未验证

- `FAIL1_EN`仍保持0；CMP/DAC输入配置、极性确认和受控注入未实现。
- `LksProtection_ClearFail()`仍返回BLOCKED，EIF的W1C清除未实现。
- Bench目前只是可读状态的阻塞诊断入口，没有真实DAC注入、有限逻辑PWM和完整P0–P4序列。
- Mock回归未执行；完整Keil构建/链接未完成；上述源码实现均未获得运行或硬件验证。
- 不修改SDK来修补HAL注释，不解除`config_valid=0`及RUN/MOE置1的安全阻塞。

后续建议：完成Mock状态机测试、隔离目录下的完整Keil构建，并继续向厂商确认图14-6入口FCLK编号及芯片修订适用说明。

## 8. 未解决项及实测边界

厂商确认入口FCLK编号、图文分频/门控位置差异、实物修订仍待处理。采样边界相位、总延迟、停止CNT/关闭CLK_EN或模块门控后的实际滤波行为未测试。CMP窗口关闭语义、DAC/REF量程与共模、极性、持续FAIL与MOE恢复优先级仍未关闭。

本报告不新增或授权硬件用例；后续必要实测另行评审，必须满足既有P0–P4物理隔离条件，不得直接执行当前未完成Bench。P1保持BLOCKED。

## 9. 项目状态与归档边界

| 项目 | 保持状态 |
|---|---|
| V02-01设计 | DESIGN完成 |
| 固件 | IMPLEMENTED PARTIAL |
| 三个新增C模块语法检查 | 先前已通过；不是完整构建或功能测试，本轮未重跑 |
| Mock测试 | 未执行 |
| Keil完整构建 | NOT BUILT |
| 烧录／P0～P4 | 未执行／全部NOT RUN |
| config_valid／RUN | 仍为0／仍BLOCKED |
| A5后端／SDK／HAL | 本次提交差异未修改；专项审计本身也未修改这些代码 |
| Keil工程 | 提交`6dc46f3`修改`LK_StdPeriph.uvprojx`，为两个Target增加V02 Protection分组及三个C源文件；因此该提交不是纯文档提交 |

专项审计执行本身为只读，未修改固件、SDK、Keil工程，未构建、烧录或执行硬件动作。随后Git提交`6dc46f36ad61b61093059056477d91e7252c58c1`（15个文件，+764/-5）同时包含本报告及关联Markdown、`main.c`、六个保护模块C/H文件和Keil工程文件变更。该提交属于“V02-01部分实现 + 时钟审计文档”的混合提交，不能称为纯文档提交；仅凭提交本身不能判定各项代码修改的具体产生时间。本次修订前的云端读取时，GitHub `main`已指向该提交。此前本地工作区中的未提交用户配置/产物变化不属于该远端提交，也不由本报告推断其具体语义。
