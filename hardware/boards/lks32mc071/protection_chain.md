# 硬件保护链路审计

## 逐波过流目标链路

~~~text
Current Sense
   -> Amplifier / Filter
   -> Comparator (CMP or driver OCP)
   -> PWM Break / Gate Driver Disable
   -> Power MOSFET gate output forced safe
   -> Fault status captured by MCU
~~~

上图是待验证的目标链路，不代表当前板卡已经按此连接。

| 环节 | 实际器件/网络 | 阈值/极性 | 响应时间 | 证据 | 状态 |
|---|---|---|---|---|---|
| Current Sense | TBD | TBD | TBD | TBD | TBD |
| Comparator | TBD | TBD | TBD | TBD | TBD |
| PWM Break / Driver Disable | TBD | TBD | TBD | TBD | TBD |
| Fault latch / clear | TBD | TBD | TBD | TBD | TBD |

## 必须回答
1. 比较器是否能不经过软件中断直接关断 PWM？
2. 关断覆盖所有桥臂吗？
3. 故障是逐周期自动恢复还是锁存？
4. MCU 复位或失控时，驱动器默认处于什么状态？
5. 硬件阈值是否低于功率器件和采样电路的安全上限？
6. 过压时是否存在母线能量吸收路径？

完成电路核对后，必须进行限流条件下的受控验证。
