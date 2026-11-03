# V01-PWM-001 — 六步 PWM 输出验证

- Requirement ID: SYS-COMM-001
- Milestone: V0.1
- Status: NOT RUN

## Objective
验证六种换相状态、PWM 极性、死区、互锁和安全关闭。

## Preconditions
- V0.0 硬件映射完成。
- 功率级使用安全低压测试条件。
- 电源限流、急停和硬件保护已确认。
- 首轮可先断开电机或使用适当假负载。

## Procedure
1. 逐状态输出六步表。
2. 测量每路 Gate 波形。
3. 检查上下桥臂死区和互锁。
4. 检查扇区切换时有无窄脉冲。
5. 触发安全关闭并确认所有输出进入预期安全态。

## Expected Result
六步状态正确，无直通风险、无非预期脉冲，安全关闭有效。
