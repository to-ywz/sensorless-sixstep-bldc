# Changelog

项目文档、架构、硬件和固件的可追溯变更记录。**Unreleased不是已发布版本；本轮未创建版本Tag/Release。**

## Unreleased

### Design decision（2026-11-16）
- 用户确认V0.2双级保护架构：**一级CBC逐波限流**负责本PWM周期截断和后续安全周期受控恢复；**二级严重故障锁存**负责持续/严重异常的停机与显式恢复。V02-01先在锁存模式验证CMP1→FAIL1异步硬件关断，不能将其当作CBC或V0.2最终PASS。
- CMP1仅一路输入和FAIL自动恢复实际能力仍需核实；两级故障源、阈值与同步恢复机制均TBD，尚未写入固件/上板验证。更新[总计划](tests/test_cases/V02-PLAN-000-workplan.md)、[首轮保护链](tests/test_cases/V02-PROT-001-internal-path-plan.md)、[完整逐波验收](tests/test_cases/V02-PROT-001-cycle-current-limit.md)。

### Documentation / planning（2026-11-16）
- 将A5验收用例整理为单一状态矩阵：[A5验收](tests/test_cases/V01-A5-driver-interface-gate.md)与[当前证据表](tests/reports/2026-11-15_V01-A5-progress.md)。保留原始分阶段报告/截图哈希，不合并或覆写历史证据。
- 分清U/V/W HIN/LIN输入非重叠、Gate对地约202µs、复位稳定VGS、六管**停止状态**电源循环和U相下管**导通中**掉电；这些不是同一次/同一种测试。
- 保留[HW-001](hardware/issues/HW-001-mcpwm-edge-ringing.md)；新增/整理[HW-002](hardware/issues/HW-002-powerdown-gate-discharge.md)跟踪失电VGS渐降与供电残压。
- 新增[V0.2采样与CMP1→FAIL1阶段工作计划](tests/test_cases/V02-PLAN-000-workplan.md)；PROT-001内部逻辑链仍**NOT IMPLEMENTED / NOT TESTED**，不放行主功率母线和电机。

### A5 current evidence
- 2026-11-12～16：U/V/W低侧、三相自举、高侧短包、输入逻辑交接、复位后无持续误开启及六管停止状态电源循环取得功能性证据；时序Watch诊断与以前约200～202µs波形近似一致。
- U相下管导通中关闭DM_In，观测到供电轨/VGS缓降、最终VGS接近0，拔除J-Link后现象仍在；约5秒15V残压1.2V、5V残压0.2V。HW-002尚未定位根因。
- 2026-11-16的A5隔离构建有报告，但不能证明此前实测板载固件与该HEX完全一致；A5安全验收整体仍**PARTIAL PASS**。

### Earlier foundation
- 建立Target V1.0需求、LKS32MC071硬件审计、V0.0～V1.0路线和测试模板。
- 建立A1～A5独立台架及共享安全输出极性修复；详细源码/测试变更以各阶段Git提交和报告为准。

### Not verified / open gates
- 全六扇区真实电流采样及比较器过流阈值、CMP1 RAW→MCPWM FAIL1实物链、cycle-by-cycle恢复。
- LKS560输入尖峰电气兼容、复位/失电纳秒瞬态与实际双VGS非重叠、功率级安全放行。
- 尚无经过本项目完整验证的无感闭环电机控制固件。
