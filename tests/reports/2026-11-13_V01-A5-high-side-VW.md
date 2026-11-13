# V01-A5 V/W相高侧短包复测（2026-11-13）

- Status: THREE-PHASE HIGH-SIDE SHORT-PACKET FUNCTION OBSERVED NORMAL; A5 REMAINING ITEMS OPEN
- 用户反馈：稳定，正常。
- 相位对应：按上一轮要求的测试顺序暂记图1为V、图2为W；截图未显示相位变量，不能由波形本身识别相位。
- 条件：沿用母线0V、电机断开、C1为本相高侧Gate、C2为同管Source、两探头地夹同一GND的既定方法；截图不独立证明接线及供电条件。

## 观测与结论

两张图均显示Math2=CH1−CH2，两个约14V量级VGS脉冲高度接近、平台无明显下垂，结束后回到接近0V。脉冲间距视觉上约100µs，与10kHz短包一致。结合此前U相结果，支持三相高侧短包功能正常。

时基100µs/div，6.25MSa/s、10kpts、160ns/pt，C1/C2为10V/div、20MHz带宽限制。图1Max(C1)=14.510V、Max(C2)=1.0040V；图2分别14.542V、1.0520V。两个独立Max不作相减来构造VGS峰值。

本轮不关闭200µs窗口精度、微秒级边沿/尖峰、实际上下管非重叠、复位/掉电关断、输入峰值兼容性等独立验收项，也不授权带母线或连接电机。下一项应聚焦预充低侧退出到高侧开启的实际栅极交接，确认两管无同时导通；这与单独看到高侧短包正常是不同证据。

## 原图索引

原PNG按要求命名保存在Git忽略的本地raw目录，未修改像素；对应关系依据已写入manifest。

- V：`hardware/measurements/raw/2026-11-13_V01-A5/2026-11-13_V01-A5_01_V_high-side_VGS_CH1-minus-CH2.png`；SHA-256：`59e74e14ae7feb504c18610fab027600f498c579255fa46806783b32232c87cb`。
- W：`hardware/measurements/raw/2026-11-13_V01-A5/2026-11-13_V01-A5_02_W_high-side_VGS_CH1-minus-CH2.png`；SHA-256：`2b0f25a04d2bba4226cdafdb1d3078dbde77968b4e4d5b9c43d91db6e90ad698`。
