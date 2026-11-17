# V02-01 工作包1：Protection Manager Mock 回归

- 实测日期：2026-11-17；版本记录日期：2026-11-18。
- Source baseline：493ab126082c3c3fc38500d58c57c49877ecc395。
- Result：V02-01 SOFTWARE MOCK REGRESSION PASS。
- 实际仓库为F:/Demo/sensorless-sixstep-bldc；任务D盘目录不存在，未切分支或重置。

## 方法与范围

编译真实protection_manager.c及接口，仅以Fake实现六个平台函数；不编译真实lks_protection.c，不复制状态机。每个场景独立进程启动，无生产Reset接口。Fake覆盖初始化/快照有效性/RAW/EIF/MOE/读写失败/CLEAR重现/开启过程异步FAIL，以及PRIMASK保存恢复和嵌套。主机合格场景不改变真实硬件资格。

SW-01～SW-10及附加异常共45个独立场景（31个主要求子场景、14个附加场景）全部通过，完整软件回归执行2轮；旧有四组各通过2轮。C99 -Wall -Wextra -Werror，零编译警告。临时副本中10种故意错误均被对应测试捕获。首次辅助变异试跑遇到Windows CRT断言超时，未计通过；测试改为无弹窗、失败非零、NDEBUG不可绕过的CHECK后完成10/10对照。

## 逐项结果

| 编号 | 结果 | 覆盖 |
|---|---|---|
| SW-01 | PASS | SDK清EIF前boot历史保留，first_fault为另一时点，不允许RUN |
| SW-02 | PASS | 真实未获资格配置镜像及无效RAW/配置/不可读/blocker分别阻塞；真实平台仅源码门禁检查 |
| SW-03 | PASS | 显式RUN/STOP、MOE状态及重复RUN，不产生错误锁存 |
| SW-04 | PASS | 有效RAW故障关断、锁存、首次现场 |
| SW-05 | PASS | RAW正常但EIF故障仍关断锁存 |
| SW-06 | PASS | RAW/EIF/软件锁存分别拒绝RUN且不发开启MOE |
| SW-07 | PASS | SetMoe(1)内异步故障即使PRIMASK=1仍注入，后检查捕获；不证明硅片无瞬时窗口 |
| SW-08 | PASS | 源撤销后Process/STOP/RUN不自动清除或重启 |
| SW-09 | PASS | 无效/有效故障RAW、MOE1、非锁存态、不可读拒绝CLEAR，原锁存信息不变 |
| SW-10 | PASS | CLEAR复查，成功只DISARMED；明确RUN才开启；清除失败或故障/资格重现仍锁存 |
| 附加 | PASS | 重Init、MOE部分写入/失败/无读回、关断失败IO_ERROR、异常MOE、首次现场保留、NULL、嵌套临界区 |

没有发现已覆盖场景中需生产逻辑修复的问题；生产Manager及其头文件未修改。first_fault保证当前锁存期首次记录，成功CLEAR后的新故障期可重新记录；不承诺永久多事件日志。reason可能更新或在latched=0后保留历史，调用方先看latched。

## 限制与安全状态

真实平台config_valid/raw_valid/hardware_qualified仍0，FAIL1_EN=0，RUN/MOE开启及CLEAR BLOCKED，P0-P4 NOT RUN。只读源码检查不替代真实平台执行。Mock不证明CMP极性、电气条件、物理关断、异步硬件重开窗口或CBC/OCP合格。

当时没有Keil构建、烧录或硬件操作。完整本地报告、变异记录及任务diff留在本机v02-01-review目录。2026-11-18用户另行授权做版本管理，故将测试与本文入库；先前工作包本身没有擅自提交。

## 命令及最终轮控制台

运行：python tests/unit/run_lks560_tests.py --cc <GCC路径>。实际使用MinGW GCC13.2.0；推荐从仓库根目录执行。源文件及测试在版本管理前已与验证副本逐字节比对一致，无后续代码修改，复用既有通过结果。

```text
PASS: blocked platform SOURCE guard (not hardware): config/raw/hardware qualification=0, FAIL1_EN default=0, RUN/MOE/CLEAR blocked; P0-P4 not executed
PASS: SW-01-boot-history
PASS: SW-02-current-blocked
PASS: SW-02-invalid-raw
PASS: SW-02-invalid-config
PASS: SW-02-unreadable
PASS: SW-02-blockers
PASS: SW-03-run-stop
PASS: SW-04-raw-fault
PASS: SW-05-eif-fault
PASS: SW-06-run-raw
PASS: SW-06-run-eif
PASS: SW-06-run-latched
PASS: SW-07-async-fail
PASS: SW-07-raw-during-enable
PASS: SW-07-eif-with-moe-high
PASS: SW-08-no-auto-restart
PASS: SW-09-clear-invalid-raw
PASS: SW-09-clear-active-raw
PASS: SW-09-clear-moe-high
PASS: SW-09-clear-not-latched
PASS: SW-09-clear-unreadable
PASS: SW-09-clear-running
PASS: SW-10-clear-explicit-rerun
PASS: SW-10-clear-blocked
PASS: SW-10-clear-io-error
PASS: SW-10-clear-eif-reassert
PASS: SW-10-clear-raw-reassert
PASS: SW-10-clear-moe-reassert
PASS: SW-10-clear-raw-invalidated
PASS: SW-10-clear-config-invalidated
PASS: SW-10-clear-read-lost
PASS: EX-reinit-keeps-latch
PASS: EX-enable-partial-failure
PASS: EX-enable-blocked
PASS: EX-disable-failure-visible
PASS: EX-fault-disable-failure
PASS: EX-disarmed-unexpected-moe
PASS: EX-first-fault-preserved
PASS: EX-null-fault-info
PASS: EX-primask-already-disabled-nested
PASS: EX-enable-no-write
PASS: EX-run-config-lost
PASS: EX-run-read-lost
PASS: EX-init-io-error
PASS: EX-fault-before-blocked
Protection Manager: 45 isolated scenarios PASS
sixstep table, forward/reverse cycles, invalid inputs and duty limits: PASS
bench debounce, STOP priority, config restart, 10 forward/reverse cycles, invalid input, tick wrap: PASS
PASS: A5 defaults, all phases, finite packets, STOP priority, bootstrap config, late polling, invalid inputs, tick wrap
PASS: real A0-A4 sources, LKS560 physical input/gate mapping, GPIO STOP, MOE defaults, invalid inputs and STOP-before-enable
```
