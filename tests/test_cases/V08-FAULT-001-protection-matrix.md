# V08-FAULT-001 — 保护矩阵故障注入

- Requirement ID: PROT-*
- Milestone: V0.8
- Status: NOT RUN

## Objective
逐项验证 cycle、stall、voltage、temperature、phase、overspeed 和 reverse 保护。

## Preconditions
- 每种故障已有安全、可控的注入方法。
- 保护阈值和恢复条件已评审。
- 不使用危险的直接短路或超额定电压操作。

## Procedure
对每类故障分别记录触发条件、检测延迟、PWM 动作、故障码、锁存/恢复行为和重复性。

## Expected Result
每种故障均按规定动作；无无限重试；故障消失后也不会在未满足恢复许可时自动重新输出。
