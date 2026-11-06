# V01-A0 — 最小 BSP / 控制板安全态验收

- Status: **USER-REPORTED BUILD PASS / BUILD LOG PENDING / HARDWARE NOT TESTED**
- Target: LKS_EVB_MCU071CBT8_V1.0
- GitHub 提交版本：8fef844a（Keil 工程加入；用户随后口头确认已成功编译）
- Keil 主目标：Linko 07x Project（MCU：LKS32MC071CBT8）；不要选用尚未核实 Include Path 的 Linko 07x Project_ac6。
- Vendor peripheral driver: V3.16
- Reference Keil demo: V1.9 / 014_GPIO

## A. 编译审核

- [ ] 恰好一个 `main()` 和一个 `SystemInit()`；已排除原版 main 和 hardware_init。
- [ ] 使用 071 对应的 CMSIS、startup、SDK；原厂 NVR/trim 等相关依赖已链接。
- [ ] 编译通过，无未定义函数或重复定义。
- [ ] P1.4..P1.9 保持 GPIO AF0，未启用 MCPWM 外设。
- [ ] P0.6 的标注与当前控制板图纸一致。

## A1. 编译成功的证明（请由用户提交）

- [ ] 在 Keil 的 Build Output 记录所选 Target（建议 Linko 07x Project）。
- [ ] Build Output 明确出现 `main.c`、`board_init.c`、`system_startup.c` 编译记录，且 0 Error；`AppFunction/hardware_init.c`、`Kernal_Source/main.c` 不参与编译。
- [ ] Build/Rebuild 后本次 `Objects/LK_StdPeriph.hex` 文件修改时间变化；不能使用 Git 仓库中旧的 HEX 作为新构建通过的证据。
- [ ] 若有警告，保留警告全文，优先判断原厂代码/用户代码来源。
- [ ] 保存 Build Output 截图或文本；只审核编译产物不代表硬件已测试。

## B. 上电实测（电机未连接，主功率桥未供电）

- [ ] 独立供 MCU 控制板逻辑电源，电压、极性、限流和 GND 已确认。
- [ ] SWD 能连接、停止和单步运行。
- [ ] P0.6 观察到周期性翻转（只用于程序运行指示）。
- [ ] P1.4、P1.5、P1.6、P1.7、P1.8、P1.9 均测得低电平。
- [ ] 上电和复位期间六路信号状态另行记录，不默认认定安全。
- [ ] MCPWM 输出功能未打开；无非预期高脉冲。

## C. 测试报告

- 日期与板卡版本：
- Keil/ARM 编译器版本与日志：
- SDK 实际依赖：
- 调试器：
- 逻辑供电：
- P0.6 实测波形：
- P1.4..P1.9 电平记录：
- 复位期间电平记录：
- 结果（NOT TESTED / PASSED / BLOCKED）：

A0 未通过前，不进入 A1，不接 FN550-90K，也不将功率桥供电。
