# 电机状态机

## 1. 状态流

~~~text
INIT -> IDLE -> PRECHECK -> ROTOR_DETECTION
                              |
                 +------------+-------------+
                 |                          |
              STATIONARY                  MOVING
                 |                          |
             ALIGN / IPD              FLYING_START_CHECK
                 |                          |
                 v                          v
             OPEN_LOOP_ACCEL <--------- SYNC_CANDIDATE
                 |
          BEMF_VALIDATION
                 |
          CLOSED_LOOP_RUN
                 |
             STOPPING
                 |
               IDLE

Any state -- unrecoverable fault --> FAULT
~~~

## 2. 状态定义
- INIT：外设初始化，PWM 保持安全关闭。
- IDLE：等待启动命令。
- PRECHECK：检查母线、电流偏置、故障输入和参数有效性。
- ROTOR_DETECTION：判断静止、同向旋转或反向旋转。
- ALIGN：静止转子预定位。
- IPD：可选脉冲式初始位置检测。
- OPEN_LOOP_ACCEL：按受控频率/换相周期斜坡强制换相。
- FLYING_START_CHECK：验证 BEMF 连续性、方向和转速是否可接管。
- BEMF_VALIDATION：验证过零序列与开环扇区/相位关系。
- CLOSED_LOOP_RUN：BEMF 同步换相和速度闭环。
- STOPPING：受控停止并关闭驱动。
- FAULT：按故障等级锁存或等待恢复条件。

## 3. 转换约束
- 未通过 PRECHECK 不得启动 PWM。
- ALIGN/IPD/Open-loop 期间仍受硬件逐波过流保护约束。
- BEMF 未连续有效前不得切换到闭环换相。
- 方向不一致时不得直接接管。
- 方向切换必须走安全停止流程，除非未来单独设计并验证动态反转策略。
- FAULT 清除必须检查故障原因已消失。
- 启动重试次数有限，禁止在堵转状态无限重复冲击。
