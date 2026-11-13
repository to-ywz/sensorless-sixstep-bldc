# Board Profiles

每块板的配置说明放在现有 `hardware/boards/<board>/`，按实物修订版本维护；当前第一套参考为 [LKS32MC071](lks32mc071/README.md)。沿用其 pin_mapping.md、signal_mapping.md、sampling.md、protection_chain.md，不另复制参数库。

新增/完善 Profile 时应覆盖：

- 板号、PCB/装配修订、控制板与功率板组合、资料版本与确认来源。
- MCU/时钟、引脚及复用、PWM 通道/极性、使能/故障/复位默认态。
- 电源轨、上电顺序、母线和驱动电源、限流及允许供电组合。
- 电流/电压/BEMF/温度采样拓扑、比例、偏置、单位和标定依据。
- 预驱型号、输入极性、自举、Gate 及硬件保护链路。
- 测试点、网络、探头接地/差分参考、仪器接法。
- 电压/电流/温度/占空比/时长安全限值、停止条件和电机连接限制。

每项区分 CONFIRMED / INFERRED / TBD / TESTED，附来源或证据。未知值保持 TBD，依赖它的测试标记 BLOCKED；不得把示例或芯片额定值直接当作板级安全工作限值。
