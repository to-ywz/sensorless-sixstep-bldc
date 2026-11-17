/**
 * @file bench_cmp_fail.h
 * @brief 独立于A5的保护诊断入口，当前不具备注入测试功能。
 * 使用BENCH_TEST_CMP_FAIL选择，与其他Bench宏互斥。
 * 不允许将BLOCKED报告解释为硬件测试PASS。
 */
#ifndef BENCH_CMP_FAIL_H
#define BENCH_CMP_FAIL_H
#include "../../application/protection/protection_manager.h"
typedef enum { BENCH_BLOCKED, BENCH_PASS, BENCH_FAIL } CmpFailTestResult;
typedef struct {
    /* 四类用例分别记录；当前无硬件资格，默认均BLOCKED。 */
    CmpFailTestResult raw, hardware_fail, latch, clear_run;
    ProtectionResult init_result, stop_result;
    ProtectionSnapshot registers;
    ProtectionFaultInfo fault;
    /* 仅表示本次诊断记录填写结束，不代表四类测试完成或通过。 */
    uint32_t completed;
} CmpFailBenchReport;
/* 调试器观察入口；volatile保证写入可见，但整份报告不是原子快照。
 * poll只监视Manager，不周期刷新此初始化报告。 */
extern volatile CmpFailBenchReport bench_cmp_fail_report;
void bench_cmp_fail_init(void);
void bench_cmp_fail_poll(void);
#endif
