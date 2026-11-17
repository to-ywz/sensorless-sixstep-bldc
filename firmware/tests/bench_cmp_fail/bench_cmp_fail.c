/**
 * @file bench_cmp_fail.c
 * @brief 记录初始化、STOP及快照，不写MOE，不调用A5后端。
 * 上层main先执行board_init_safe。未来硬件注入须另行满足物理隔离门槛；
 * 当前入口没有DAC注入、PWM输出、延迟测量或完整P0-P4序列。
 */
#include "bench_cmp_fail.h"
volatile CmpFailBenchReport bench_cmp_fail_report;
void bench_cmp_fail_init(void)
{
    ProtectionSnapshot s;
    ProtectionFaultInfo f;
    bench_cmp_fail_report.completed=0u;
    bench_cmp_fail_report.raw=BENCH_BLOCKED;
    bench_cmp_fail_report.hardware_fail=BENCH_BLOCKED;
    bench_cmp_fail_report.latch=BENCH_BLOCKED;
    bench_cmp_fail_report.clear_run=BENCH_BLOCKED;
    /* 即使初始化BLOCKED，也发STOP并记录其原因；不静默忽略失败。
     * 四个测试状态不因初始化返回而自动改为PASS。 */
    bench_cmp_fail_report.init_result=Protection_Init();
    bench_cmp_fail_report.stop_result=Protection_RequestStop();
    LksProtection_Read(&s);
    Protection_GetFaultInfo(&f);
    bench_cmp_fail_report.registers=s;
    bench_cmp_fail_report.fault=f;
    /* Debugger-visible report. No injection, pin AF routing or RUN while blocked. */
    /* MOE读数为1时报告异常；读数无效时仍为BLOCKED而非PASS。
     * 此处没有测量六路pad，不可用MOE值替代物理默认态验收。 */
    if (s.moe) bench_cmp_fail_report.hardware_fail=BENCH_FAIL;
    bench_cmp_fail_report.completed=1u;
}
void bench_cmp_fail_poll(void)
{
    (void)Protection_Process();
    /* This blocked bench never requests RUN; manager retains safe shutdown. */
}
