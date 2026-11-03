# 软件架构与接口边界

## 1. 分层职责

### Application
用户命令、目标速度、方向请求、参数装载、诊断输出和产品级接口。

### Motor Core
状态机、运行模式、启动流程协调、换相流程协调、故障状态协调。不得包含芯片寄存器访问。

### Algorithm
- startup：ALIGN、IPD、open-loop ramp、flying start。
- commutation：六步扇区和换相调度。
- bemf：过零检测、滤波、消隐和去磁判定。
- estimator：速度和方向估算。
- control：速度 PI、斜坡、限幅。
- protection：软件故障检测和恢复管理。

### Board
当前功率板的引脚映射、采样转换系数、比较器配置、驱动器使能极性和硬件能力声明。

### Platform
LKS32MC071 的 MCPWM、ADC、CMP、Timer、GPIO、IRQ、Watchdog 等外设适配。

## 2. 依赖规则

~~~text
Application -> Motor Core -> Algorithm Interfaces
                         -> Board Interfaces
Board -> Platform
Platform -> MCU SDK / Registers
~~~

Algorithm 不得依赖 Platform 头文件或寄存器定义。Platform 不得包含电机策略。

## 3. 建议接口（概念层，非最终 API）
- pwm_apply_sector(sector, duty, mode)
- pwm_output_enable(enable)
- pwm_force_safe_state()
- adc_get_phase_voltage(phase)
- adc_get_bus_voltage()
- adc_get_current()
- timer_get_timestamp()
- comparator_get_fault_status()
- board_get_motor_parameters()
- board_get_sensing_parameters()

实际接口在 V0.0 硬件审计和 V0.1 驱动设计后冻结。

## 4. 参数分组
- Board Config：PWM 极性、死区、ADC 通道、比例、偏置、CMP 阈值。
- Motor Config：极对数、相序、方向、电机电气参数、允许电流和速度。
- Algorithm Config：ALIGN 时间、IPD 参数、加速曲线、消隐、滤波、PI 参数。
- Protection Config：阈值、滞回、延时、重试次数。

所有参数必须标注单位、有效范围、来源和是否已实测。

## 5. ISR 与主循环边界
- PWM 更新、CMP 故障、关键时间戳捕获可由 ISR 完成。
- ISR 只做有界、确定性工作；复杂策略放到可控调度上下文。
- 对跨 ISR / 主循环共享的计数器和结构体，明确原子访问策略。
- 故障关断不依赖软件事件队列或主循环轮询。
