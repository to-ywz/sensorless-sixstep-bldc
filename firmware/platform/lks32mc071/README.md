# LKS32MC071 Platform

实现 MCU 相关外设适配：时钟、MCPWM、ADC、CMP、Timer、GPIO、IRQ、Watchdog。

要求：
- 封装寄存器访问。
- 提供可测试的初始化和状态读取接口。
- 明确 PWM 安全默认态。
- 明确 ADC 触发源、采样时间和中断优先级。
- 明确 CMP 到 PWM Break / Driver Disable 的硬件路径。
- 不在此层实现电机启动、BEMF 算法或速度 PI。


## V02-01 保护接口边界（设计已归档，尚未实现）

CMP1 / MCPWM FAIL 的寄存器操作、状态快照、FAIL事件清除和MOE控制属于本平台层；电机运行状态、故障锁存策略和显式RUN/CLEAR命令属于Application层。后续应由Protection Manager成为唯一允许请求MOE开启的上层入口，Bench与产品逻辑复用平台保护接口，不允许A5或其他输出后端绕过故障锁存直接写MOE。

当前仅有设计文档：[V02-01平台接口与锁存状态机](../../tests/test_cases/V02-01-platform-interface-state-machine.md)。本文件不是已实现API说明；没有新增源文件、没有修改A5后端，也没有构建或测试。
