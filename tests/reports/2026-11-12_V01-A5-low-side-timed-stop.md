# V01-A5 新固件低侧限时/手动停止实测（2026-11-12）

- Status: THREE-PHASE LOW-SIDE TIMED HOLD AND MANUAL STOP USER-CONFIRMED PASS
- Firmware: BENCH_TEST_GATE_DRIVER，g_a5_test=0；实际烧录指纹未另行提供。
- Phase: 用户明确三张图的g_a5_phase依次为0、1、2，即U、V、W；不是1、2、3。
- Conditions: 沿用P3摘掉、DM_In独立供电、母线0V、电机断开方案；本轮未重新提供供电数值。
- Measurement: 沿用低侧VGS测量任务；C1显示。探头实际接法/衰减倍率本轮未另附照片，不由纵轴刻度推断。
- Equipment: RIGOL DHO914，截图显示2 s/div、10 V/div、50.00 kSa/s、1.00 Mpts、20 µs/pt、20 MHz带宽限制。

## 实测结果

| 相 | g_a5_phase | 自动结束 | 手动STOP | 屏幕最大值 |
|---|---:|---|---|---:|
| U | 0 | 第一段约5s后回约0V | 第二段提前结束回约0V | 15.212V |
| V | 1 | 第一段约5s后回约0V | 第二段提前结束回约0V | 15.188V |
| W | 2 | 第一段约5s后回约0V | 第二段提前结束回约0V | 15.234V |

三张图均显示稳定高电平段约15V，自动和手动结束后保持接近0V。最大值是仪器Max读数，不是平均导通电压；无时宽光标，不声称5s达到毫秒级误差要求。手动STOP操作及先后顺序由用户说明，不能从屏幕STOP采集状态本身推断按键操作。

## 证据索引

原PNG按日期/测试编号/序号/相位/用途命名，原样保存在本地 `hardware/measurements/raw/2026-11-12_V01-A5/`，Git忽略；low-side-manifest.json保存来源、相位和SHA-256。

- `2026-11-12_V01-A5_01_U_low-side_auto-manual-stop.png` — SHA-256 `bc58cf6fa0600c492aefd30a49cb2fcf77836321cb2189a1afe6c984dcbf2244`。
- `2026-11-12_V01-A5_02_V_low-side_auto-manual-stop.png` — SHA-256 `dcae2073d450dad50b737c0ebda6cf1139da49f33ef24a22d6a6193e21cbdf8b`。
- `2026-11-12_V01-A5_03_W_low-side_auto-manual-stop.png` — SHA-256 `769f68667aeb79da79150fe6301fe4bad71f863daeb6f790b0574c1ec9cf0543`。

## 验收范围与后续

A5新固件三相低侧5s限时导通及手动STOP功能已由截图和用户反馈支持；本轮未独立量化STOP延迟、逐项记录未选中其它相的栅极，也未覆盖复位高阻或真实上下管死区。

下一项为本相低侧导通时测驱动器VB−VS，确认自举供电。未测前保持g_a5_test=0，不设置bootstrap_checked，不执行高侧短包。高侧、CMP1保护链及主母线功率运行仍待验证。
