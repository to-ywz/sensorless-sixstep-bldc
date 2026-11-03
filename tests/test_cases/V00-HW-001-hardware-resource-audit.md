# V00-HW-001 — 硬件资源审计

- Requirement ID: SYS-BASELINE
- Milestone: V0.0
- Status: NOT RUN

## Objective
确认 MCU、核心板、功率底板、PWM、ADC、CMP、驱动器和保护链路的实际连接。

## Preconditions
取得当前版本原理图、PCB、BOM、芯片手册；确认实物板卡丝印和版本。

## Procedure
1. 核对 MCU 完整型号和封装。
2. 建立六路 PWM 引脚表。
3. 追踪电流采样到 ADC/CMP 的链路。
4. 追踪 U/V/W BEMF 和参考电压链路。
5. 追踪 VBUS 和温度采样。
6. 追踪 CMP/Driver Fault 到 PWM 关断的路径。
7. 为每条信号记录原理图页码和网络名。

## Expected Result
所有关键功能有明确网络、器件、引脚和证据；不确定项标记 TBD，不以推断代替确认。
