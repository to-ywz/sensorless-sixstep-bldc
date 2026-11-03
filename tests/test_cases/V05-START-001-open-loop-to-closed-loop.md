# V05-START-001 — 开环到闭环换相接管

- Requirement ID: SYS-START-006
- Milestone: V0.5
- Status: NOT RUN

## Objective
验证开环启动能够在 BEMF 有效后平稳切换到闭环换相。

## Preconditions
V0.3 开环启动和 V0.4 BEMF 检测均已通过。

## Procedure
1. 记录开环扇区、换相周期和 BEMF 事件。
2. 记录闭环判定窗口和接管时刻。
3. 观察接管前后相电压、电流、换相时间和速度。
4. 在不同初始位置和负载下重复测试。
5. 验证 BEMF 无效时拒绝接管并安全退出。

## Expected Result
接管后换相顺序正确，无持续失步或不可接受电流冲击；失败条件可识别。
