# Firmware · V0.1 分阶段逻辑级验证

**当前状态（2026-11-08）：A0 最小 BSP、A1 单路 MCPWM、A2 单组互补 PWM 均有用户实测通过记录；A3 三组互补 PWM 待验证。** 仓库未归档完整 Build Output / 示波器原图；**驱动器、功率级、CMP/Fail 硬件 OCP 和电机运行均未通过验收**。所有 A0～A3 逻辑测试都要求断开电机及主功率母线。

## 组织架构

- `application/main.c`：最小运行入口、心跳。
- `board/lks_evb_mvpowpre/board_init.c/.h`：当前 LKS_EVB_MCU071CBT8_V1.0 板级初始化。
- `platform/lks32mc071/system_startup.c`：官方启动文件调用的 `SystemInit()`，在 C 运行时初始化前不切换时钟。
- 后续 motor/{core,startup,commutation,bemf,estimator,control,protection} 各模块按实际测试进度启用，不一次性集成。

## SDK / 示例工程依赖与当前提交状态

已检查用户此前提供的两份原厂 ZIP：
- `lks32mc07x_periph_driver_v3.16.zip`
- `lks32mc07x_demo_prj_v1.9.zip`

选择官方 **014_GPIO** Keil 工程作为本地工程容器（不要直接修改原厂样例，先另存副本），继续使用它的 Device/CMSIS/Startup、`lks32mc07x_gpio.c`、`lks32mc07x_sys.c` 及原厂 NVR/trim 等链接依赖。

### 2026-11-06 Keil 工程集成记录（历史基线；当前测试进度见页首）

用户已经把一份官方 Demo / Keil 工程提交到 `firmware/projects/keil/lks32mc071/`，其中包含整套 `lks32mc07x_periph_driver/`、旧的 `Kernal_Source/main.c` 和 `AppFunction/hardware_init.c`，还有本地生成的 HEX、LST、Keil 用户配置等。**不要把本节原来的集成步骤误认为还没做。**

在当前 `LK_StdPeriph.uvprojx` 主目标 **Linko 07x Project** 中：
- 芯片是 `LKS32MC071CBT8`，Keil Pack `Linko.LKS07x.1.2.0`；
- 实际编译的主程序是 `../../../application/main.c`，而不是 `Kernal_Source/main.c`；
- 实际编译的初始化是 `../../../platform/lks32mc071/system_startup.c` 和 `../../../board/lks_evb_mvpowpre/board_init.c`，旧 `AppFunction/hardware_init.c` 不在主目标源文件列表；
- 仍包含 `AppFunction/interrupt.c` 和厂商 GPIO/SYS 等驱动；
- 第二个目标 `Linko 07x Project_ac6` 的头文件路径指向 `LKS32MC08x_Periph_Driver`，当前未做兼容性验收，**不要选它作为 A0 实验目标**。

用户报告已在本地编译成功，但仓库尚无完整 Build Output 日志，不能独立确认追踪到的 `Objects/*.hex` 均由对应最新源码生成。**本段仅是 11 月 6 日的工程状态记录，后续 A0～A2 逻辑实测结果应以独立测试记录为准。**

**不要在 A3 前随意迁移 SDK、删除旧文件或改写 Keil 源码引用**；继续使用已验证 A0～A2 的主目标配置。后续清理旧示例源文件应在独立分支评估，且须确保能够回退到可工作的构建。SDK 内含 `LICENSE`（BSD 3-Clause）及 `SLA_2025.pdf`，任何重新分发或重组先核实两份许可文件及实际适用范围。

仓库根目录 `.gitignore` 已覆盖 Keil 临时配置和构建产物；**已经被 Git 追踪的文件不会因为新增 .gitignore 自动停止追踪**。必须在确认编译工程可恢复、保留必要证据之后，单独执行取消跟踪操作。


### Keil 集成步骤

1. 本地复制原厂 `014_GPIO` 工程作为项目工作副本。
2. 在 Keil 项目中**排除旧版** `Kernal_Source/main.c`、`AppFunction/hardware_init.c`。后者定义 `SystemInit`，与本仓库 `system_startup.c` 不能并存。保留所需的官方外设驱动及芯片 startup 汇编。
3. 添加本仓库 `application/main.c`、`board/lks_evb_mvpowpre/board_init.c`、`platform/lks32mc071/system_startup.c`。
4. 在头文件搜索路径加入 `firmware/board/lks_evb_mvpowpre`，并保留原有 SDK include 路径。
5. 编译；如果出现 `Read_Trim` 等链接错误，检查原厂 NVR 链接依赖，而非自己重新实现这些厂家 API。
6. 只有在**功率板主功率完全断电、没有电机连接**的条件下，以经确认安全的逻辑电源进行烧录与测试。

## A0 行为与安全约束

- MCU 上电后调用 `board_init_safe()`；使用 SDK 的 `SYS_MclkChoice(SYS_MCLK_48M_RC)`，目标为内部 RC 48MHz（需要实测确认）。
- P1.4..P1.9（对应三相六路 HIN/LIN）保持 **GPIO 模式输出低电平**；**不选择 `AF3_MCPWM`、不调用 `PWMOutputs(ENABLE)`**。
- P0.6 配置为低速翻转的 LED1 / 心跳信号。若控制板独立供电时 LED 不亮，以示波器/调试器读引脚为准。
- 其余 ADC/OPA/CMP/DAC/MCPWM 外设暂不初始化；A0 的外设安全状态需要硬件验收，不是运行程序就自动证明。
- 测试期暂时禁用内部 IWDG，后续电机运行版必须独立设计看门狗和故障安全策略。
- 软件延时不是定时器，不用其翻转周期验证 48MHz 时钟精度。

**复位期间或 GPIO 初始化之前，六路信号的默认状态仍需实测。** 代码让 GPIO 输出低电平不代表功率驱动器真实 Gate 关断已确认，也不能解除电机功率实验的硬件保护门槛。

当前仓库已有 A0～A2 用户实测记录（逻辑级）；缺少完整构建/波形归档，不可据此认定预驱、Gate、电流采样或功率级已验证。**下一步：A3 六路互补 PWM 逻辑级测试**，详见 `tests/test_cases/V01-A3-three-pair-complementary.md`。
