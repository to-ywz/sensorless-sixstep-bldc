# 采样链路与标定

所有比例系数均为 TBD，禁止在未核对原理图和实测前写入固件。

| 物理量 | 传感/调理电路 | MCU 通道 | 换算关系 | 采样时刻 | 标定方法 | 状态 |
|---|---|---|---|---|---|---|
| DC Bus Voltage | 分压/滤波 | TBD | TBD | TBD | 与校准表计对比 | TBD |
| DC/Phase Current | Shunt + amplifier | TBD | TBD | PWM 同步点待定 | 零点 + 多点电流标定 | TBD |
| U/V/W Phase Voltage | 分压/保护/滤波 | TBD | TBD | 换相窗口待定 | 与差分探头对比 | TBD |
| Temperature | NTC/传感器（若有） | TBD | TBD | 周期采样 | 温度点标定 | TBD |

## 电流换算模板
I = (ADC_voltage - offset_voltage) / (R_shunt * amplifier_gain)

该式仅为通用关系。实际还需考虑放大器参考电压、双向/单向采样、ADC 参考电压、分压和滤波。所有系数必须由实际电路确定。
