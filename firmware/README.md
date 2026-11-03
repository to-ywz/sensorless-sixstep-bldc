# Firmware

本目录承载分层固件框架。初始阶段不放未经硬件核对和测试的控制算法实现。

建议结构：
- application：产品命令和任务调度。
- board：当前控制板/功率板配置。
- config：硬件、电机和算法参数。
- motor/core：状态机和电机管理。
- motor/startup：ALIGN、IPD、开环和飞车启动。
- motor/commutation：六步换相。
- motor/bemf：悬空相过零检测。
- motor/estimator：速度和方向估算。
- motor/control：速度 PI。
- motor/protection：软件故障管理。
- platform/lks32mc071：芯片外设适配。

算法层不直接访问 MCU 寄存器。
