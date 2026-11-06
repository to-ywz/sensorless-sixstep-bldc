# V01-A1 — 单路 MCPWM 输出实验（无功率驱动）

- Status: **CODE STAGED / BUILD NOT VERIFIED / HARDWARE NOT TESTED**
- 前置条件：A0 的 P1.4～P1.9 上电/复位无异常高脉冲已确认；不接 FN550-90K，功率板主母线断电。
- 目的：只验证 MCU P1.4/CH0P PWM 输出的时钟、占空比、软件禁能及引脚切换波形。不测试三相互补、驱动电路或逐波过流。

## 启用方式（默认关闭）

1. Keil 主目标使用 `Linko 07x Project`（不是 `_ac6`）。
2. 将 `firmware/tests/bench/test_mcpwm_single.c` 添加到 Keil 项目；添加头文件路径 `firmware/tests/bench`（相对于 uvprojx：`..\..\..\tests\bench`）。
3. 在 Keil **Options for Target → C/C++ → Define** 添加 `BENCH_TEST_MCPWM_SINGLE`；未定义该宏时固件仍为 A0 心跳程序。
4. Rebuild All，确认编译 0 Error；如有 warning 保存全文。
5. 保证外部主功率级没有电源且电机断开；只给控制板安全逻辑电源。
6. 示波器 CH1：P1.4 对已确认的逻辑 GND。触发设置为上升沿，测量 PWM 频率、占空比、脉冲高电平宽度。
7. 同时观察 P1.5、P1.6～P1.9 是否始终为低；测试软件停止和重新启动时的毛刺。
8. 删除 Define `BENCH_TEST_MCPWM_SINGLE` 并重新编译可恢复 A0。

## 预期值（必须以测量为准）

按时钟 48MHz、中心对齐 2400 刻度粗估：PWM 频率约 **10kHz**。TH00=-600、TH01=+600 预期形成约 25% 开窗；具体占空比和电平极性应根据手册逻辑与示波器确认，不能仅以配置值认定正确。

## 验收条件

- [ ] P1.4 有稳定的 PWM 波形，频率和占空比接近配置预期，偏差与真实时钟合理一致。
- [ ] PWM 关闭期间 P1.4 为稳定低电平，重新使能无异常窄脉冲。
- [ ] P1.5～P1.9 始终保持低电平。
- [ ] 上电、复位、测试模式切换未出现异常高脉冲。
- [ ] 未接电机、未启动任何功率桥栅极驱动。
- [ ] 已记录 scope 截图、示波器设置、Keil 编译日志和固件 commit。

A1 尚未通过前，禁止接入功率母线或电机；A1 不是硬件过流保护的验证。
