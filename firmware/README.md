# Firmware · V0.1-A0 最小 BSP

状态：**IMPLEMENTED / BUILD NOT VERIFIED / HARDWARE NOT TESTED**。此阶段仅验证控制板基础运行，**电机和功率桥必须断开，禁止母线供电**。

## 组织架构

- `application/main.c`：最小运行入口、心跳。
- `board/lks_evb_mvpowpre/board_init.c/.h`：当前 LKS_EVB_MCU071CBT8_V1.0 板级初始化。
- `platform/lks32mc071/system_startup.c`：官方启动文件调用的 `SystemInit()`，在 C 运行时初始化前不切换时钟。
- 后续 motor/{core,startup,commutation,bemf,estimator,control,protection} 各模块按实际测试进度启用，不一次性集成。

## SDK / 示例工程依赖（不上传第三方源码）

已检查用户此前提供的两份原厂 ZIP：
- `lks32mc07x_periph_driver_v3.16.zip`
- `lks32mc07x_demo_prj_v1.9.zip`

选择官方 **014_GPIO** Keil 工程作为本地工程容器（不要直接修改原厂样例，先另存副本），继续使用它的 Device/CMSIS/Startup、`lks32mc07x_gpio.c`、`lks32mc07x_sys.c` 及原厂 NVR/trim 等链接依赖。

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

当前环境没有 Keil/ARM 硬件编译及烧录条件，本仓库提交状态必须保留 BUILD NOT VERIFIED / HARDWARE NOT TESTED。
