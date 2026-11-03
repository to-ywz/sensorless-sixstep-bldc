# LKS32MC071 Control Board

## 基线
- MCU：LKS32MC071CBT8
- Control board revision：TBD
- Power board：LKS_EVB_MVPOWPRE_V5.0（待实物核对）
- SDK / firmware package version：TBD
- Clock configuration：TBD

## 待审计资源
- MCPWM：六路输出、互补关系、死区、同步更新、Break/Trip。
- ADC：电流、母线电压、三相电压、温度（若有）。
- CMP：输入源、参考阈值、极性、滤波和硬件输出路径。
- Timer：换相调度、时间戳和速度测量。
- GPIO：Driver Enable、Fault、方向/按键/调试接口。

逐项填写 pin_mapping.md、signal_mapping.md、sampling.md 和 protection_chain.md，并附原理图页码或实测证据。
