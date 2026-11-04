# LKS32MC071CBT8 引脚映射：V0.0 首轮原理图审计

来源：
- LKS_EVB_MCU071CBT8_V1.0.PDF，第 1 页
- LKS_EVB_MVPOWPRE_V5.0.PDF，第 1 页

标记：SCHEMATIC_CONFIRMED 表示原理图已确认；TESTED 仅限实物测量完成。本文所有项目尚未标为 TESTED。

## 六路 MCPWM

| 相位 | 核心板信号 | MCU GPIO（芯片脚号） | 复用功能 | 核心板 P1 针脚 | 功率板 P1 对应输入 |
|---|---|---|---|---|---|
| U 高侧 | UP | P1.4（27） | MCPWM_CH0P | 22 | U_P -> U1 LKS560 HIN |
| U 低侧 | UN | P1.5（28） | MCPWM_CH0N | 7 | U_N -> U1 LIN |
| V 高侧 | VP | P1.6（29） | MCPWM_CH1P | 21 | V_P -> U2 LKS560 HIN |
| V 低侧 | VN | P1.7（30） | MCPWM_CH1N | 6 | V_N -> U2 LIN |
| W 高侧 | WP | P1.8（31） | MCPWM_CH2P | 20 | W_P -> U5 LKS560 HIN |
| W 低侧 | WN | P1.9（32） | MCPWM_CH2N | 5 | W_N -> U5 LIN |

注意不同板卡的逻辑网络名称 UP / U_P 等写法不完全一致；这里按 P1 物理针脚对齐，两板连接方向仍需断电核对。

## BEMF 及保护

| 功能 | MCU 网络 | GPIO（芯片脚号） | 可用复用资源 | 核心板连接器 |
|---|---|---|---|---|
| U 相 BEMF | PU | P0.13（15） | ADC1_CH13 / CMP0_IP3 | P1-4 |
| V 相 BEMF | PV | P0.12（14） | ADC1_CH12 / CMP0_IP2 | P1-3 |
| W 相 BEMF | PW | P0.11（13） | ADC1_CH11 / CMP0_IP1 | P1-17 |
| OC | OC | P2.1（41） | CMP1_IP0 / ADC1_CH10 | P2-4 |
| 母线采样 | Vol_ADC | P2.9（37） | ADC0_CH12 | P2-3 |

## OPA 输入（仅追踪到 MCU 输入，内部放大倍数 TBD）

- WP- 经电阻网络 -> OPA0_IP（脚 20）；PGND -> OPA0_IN（脚 21）。
- VP- 经电阻网络 -> OPA1_IP（脚 23）；PGND -> OPA1_IN（脚 24）。
- UP- 经电阻网络 -> OPA2_IP（脚 35）；PGND -> OPA2_IN（脚 36）。

## 已识别的跨板接口标注差异

| P2 物理脚 | MCU 核心板标注 | 功率板标注 |
|---|---|---|
| 5 | P2.3 | P2.2 |
| 18 | P2.2 | P2.1 |

这两处是原理图网络名不一致，不代表已确认电气短路。需要断电通断核验，当前不应占用这些引脚实现新的功能。

## 下一步

1. 断电确认 P1 的 6 路 PWM 和 P2 关键网络是否一一对应。
2. 低能量验证 PWM 使能、安全关断和死区。
3. 检验 ADC BEMF 通道和内部 CMP 路由。
4. 核实 OC 是否存在无 CPU 参与的硬件关断路径。
