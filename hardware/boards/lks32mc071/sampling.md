# 采样链路与标定（V0.0 原理图审计初稿）

来源：LKS_EVB_MCU071CBT8_V1.0.PDF、LKS_EVB_MVPOWPRE_V5.0.PDF，第 1 页。原理图确认不等于实物标定。

## 1. 模拟采样映射

| 物理量 | 功率板信号/器件 | 核心板模拟输入 | 内部资源 | 状态 |
|---|---|---|---|---|
| U 相低侧电流 | R15=5mΩ；选择网 UP/UI | OPA2_IP/IN | OPA2_OUT -> CMP1 可选 | 跳帽/增益待实测 |
| V 相低侧电流 | R16=5mΩ；选择网 VP/VI | OPA1_IP/IN | OPA1_OUT -> CMP0 可选 | 跳帽/增益待实测 |
| W 相低侧电流 | R17=5mΩ；选择网 WP/WI | OPA0_IP/IN | OPA0_OUT -> CMP0 可选 | 跳帽/增益待实测 |
| 公共回流电流候选 | R69=5mΩ，PGND-GND | OPA3_IP/IN | OPA3_OUT -> CMP1 可选 | 总电流覆盖范围待确认 |
| U 相 BEMF | UX -> 40k -> PU | P0.13 | ADC1_CH13 / CMP0_IP3 | 比例待标定 |
| V 相 BEMF | VX -> 40k -> PV | P0.12 | ADC1_CH12 / CMP0_IP2 | 比例待标定 |
| W 相 BEMF | WX -> 40k -> PW | P0.11 | ADC1_CH11 / CMP0_IP1 | 比例待标定 |
| 母线电压 | DM_In -> 40k -> Vol_ADC | P2.9 | ADC0_CH12 | 比例待标定 |

P5/P6/P7 为三相电流采样模式选择：图纸标注 P-X 采样 MOS 内阻、P-I 采样电阻。预期 1-2 为 MOS 内阻路径，2-3 为 5mΩ shunt 路径；实际跳帽位置待照片/通断确认。

## 2. 公共 R69 与 OPA3

功率板 R69（0.005Ω/2512）位于 PGND 与 GND 之间，标注为合金采样电阻。核心板 OPA3_IP 经 R13=1k 接 PGND，OPA3_IN 经 R14=1k 接 GND，C7=1μF 形成输入滤波。

它可能测量 PGND-GND 之间的公共回流电流，但在测量前必须确认主功率电流的实际回流路径是否全部经过 R69。不要把它未经验证就称为母线总电流传感器。
核心板 C7=1μF 跨接 OPA3_IP/OPA3_IN，R13、R14 各为 1k。差分简化 RC 时间常数约 2ms，截止频率约 80Hz。因此 OPA3/R69 路径更像低带宽公共电流监测，不应直接用于 cycle-by-cycle OCP，除非后续证明实际滤波/放大链路满足所需响应时间。

## 3. CMP 资源限制

按 LKS32MC07x 手册的输入 MUX：
- CMP0 正端可选 OPA0_OUT / OPA1_OUT。
- CMP1 正端可选 OPA2_OUT / OPA3_OUT。

按 MCPWM Fail 映射：
- CMP0 Fail 对应 MCPWM CH0/1/2 P/N。
- CMP1 Fail 对应 MCPWM CH3 P/N。

因此当前 OPA2（U 相）及 OPA3（R69 公共采样）虽可进入 CMP1，但 CMP1 Fail 不直接作用于三相桥 CH0/1/2；OPA0/OPA1 可进入 CMP0，但 CMP0 一次只能选择一路。三相逐波保护覆盖方案尚未闭合。

## 4. 测试计划

| ID | 前置条件 | 动作 | 验收 |
|---|---|---|---|
| ADC-001 | 电机未连接、功率桥禁能 | 限流直流上电读取 Vbus ADC | 与外部表计一致 |
| ADC-002 | 断电 | 核对 P5/P6/P7 跳帽和 UP/VP/WP 的 1-2、2-3 通断 | 实际采样模式清晰 |
| ADC-003 | 电机未连接，隔离限幅信号源 | 对 PU/PV/PW 输入低压静态信号 | ADC 映射、比例及串扰确认 |
| ADC-004 | 断电/安全低压条件 | 测 R69 两端、PGND/GND、主输入负端之间通断 | 确认主电流回流路径 |
| ADC-005 | OPA/CMP 配置已评审 | 验证 OPA0~OPA3 增益、偏置及比较器输入选择 | mV/A 与阈值关系确定 |
| OCP-001 | 电机未连接、功率级禁能 | 逻辑级注入并观察 CMP/Fail/PWM 输出 | 无 CPU 参与的安全关断得到证据 |

所有实测记录 ADC 原始码、输入电压/电流、测试仪器、板卡版本、跳帽位置和示波器证据。没有实测的数据保持 TBD。
