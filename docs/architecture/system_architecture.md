# 系统架构

## 1. 系统边界

~~~text
User / Application Command
          |
          v
    Motor Manager
          |
  +-------+----------+----------------+
  |                  |                |
  v                  v                v
Startup           Commutation      Protection
Manager            & BEMF           Manager
  |                  |                |
  +------------------+----------------+
                     |
                     v
          Speed Estimator / Speed PI
                     |
                     v
               Board Layer
                     |
                     v
            MCU Platform Layer
                     |
                     v
        PWM / ADC / CMP / Timer / GPIO
                     |
                     v
       Gate Driver / Inverter / Motor
~~~

## 2. 模块职责
- Motor Manager：状态机、启停命令、模式切换、故障总协调。
- Startup Manager：静止启动、ALIGN、IPD、开环斜坡、飞车接管。
- Commutation：六步扇区、导通状态和换相调度。
- BEMF：悬空相选择、过零检测、消隐、去磁判断和事件有效性。
- Speed Estimator：电角速度、机械速度和有效性估计。
- Speed Control：速度 PI、斜坡、限幅和抗饱和。
- Protection：故障检测、分级、锁存和恢复管理。
- Board：具体板卡引脚、采样比例、极性和硬件拓扑。
- Platform：MCU 外设驱动和中断适配。

## 3. 关键数据流
1. PWM 定时器产生六步功率驱动信号。
2. ADC/CMP 在合适时序采集电流、母线电压和悬空相电压。
3. BEMF 模块输出有效过零事件及时间戳。
4. Commutation 模块根据事件计算下一次换相时刻。
5. Speed Estimator 更新转速估计。
6. Speed Control 根据目标速度与实际速度更新占空比请求。
7. Protection 可随时限制输出或要求停机；硬件过流链路独立快速关断。

## 4. 架构约束
- Startup 不直接写 MCU PWM 寄存器，只通过受控接口请求扇区、占空比和定时动作。
- BEMF 算法不假设固定 ADC 通道；通道映射由 Board 层提供。
- 软件保护不能替代硬件逐波保护。
- 所有时间参数统一单位（建议 us 或 timer tick，并明确换算）。
- 共享数据需明确 ISR/主循环访问规则、原子性和数据新鲜度。
