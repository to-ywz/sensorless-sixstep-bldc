# LKS32MC071 Control Board

## 基线
- MCU：LKS32MC071CBT8
- Control board revision：TBD
- Power board：LKS_EVB_MVPOWPRE_V5.0（待实物核对）
- SDK：LKS32MC07x 标准外设驱动 V3.16（当前 Keil 主目标）；官方 Demo V1.9
- A0～A2 逻辑测试时钟：`SYS_MCLK_48M_RC`，目标 48MHz；A1 MCPWM 实测 10.086kHz，时钟存在约 0.9% 的估算偏差（未进行独立基准标定）

## CMP / MCPWM 保护路由纠错

- 手册转换版 V1.70 与 SDK V3.16 显示：`MCPWM0_FAIL012.FAIL1_SEL=1` 可选择 CMP1 原始输出，对 CH0/CH1/CH2 执行硬件故障关断。**过去“CMP1 只能保护 CH3”的说法错误。**
- 当前优先验证 **CMP1→FAIL1 内部硬件通路**，不需预先从 P2.5 飞线至 P2.3；逐波限流与六扇区覆盖仍为 NOT TESTED。详见 `protection_chain.md`。

## 已知硬件问题

- [HW-001：MCPWM 输出边沿振铃与负向过冲](../../issues/HW-001-mcpwm-edge-ringing.md)：A2 测得 MCU 端约 -0.8V、预驱输入端约 -1.2V 的负向瞬态。当前底板不改线，问题保留到下一版底板设计评估；详见问题记录。

## 待审计资源
- MCPWM：六路输出、互补关系、死区、同步更新、Break/Trip。
- ADC：电流、母线电压、三相电压、温度（若有）。
- CMP：输入源、参考阈值、极性、滤波和硬件输出路径。
- Timer：换相调度、时间戳和速度测量。
- GPIO：Driver Enable、Fault、方向/按键/调试接口。

逐项填写 pin_mapping.md、signal_mapping.md、sampling.md 和 protection_chain.md，并附原理图页码或实测证据。
