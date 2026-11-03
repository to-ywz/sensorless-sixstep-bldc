# LKS32MC071 Platform

实现 MCU 相关外设适配：时钟、MCPWM、ADC、CMP、Timer、GPIO、IRQ、Watchdog。

要求：
- 封装寄存器访问。
- 提供可测试的初始化和状态读取接口。
- 明确 PWM 安全默认态。
- 明确 ADC 触发源、采样时间和中断优先级。
- 明确 CMP 到 PWM Break / Driver Disable 的硬件路径。
- 不在此层实现电机启动、BEMF 算法或速度 PI。
