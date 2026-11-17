/**
 * @file protection_manager.h
 * @brief 运行权限和软件故障锁存的唯一管理入口。
 * 请求同步执行，不排队、不自动重启。RESET后的RAM锁存不跨复位保存。
 * 本模块不替代硬件异步关断；当前平台阻塞，无法进入有效RUN流程。
 */
#ifndef PROTECTION_MANAGER_H
#define PROTECTION_MANAGER_H
#include "../../platform/lks32mc071/lks_protection.h"
/* BOOT_SAFE:未初始化；CONFIGURING:配置中；DISARMED:等待显式RUN；
 * RUNNING:已请求输出；FAULT_LATCHED:软件锁存；CLEAR_CHECK:清除复核。
 * BLOCKED是返回原因而非独立状态，当前通过FAULT_LATCHED保持关闭。 */
typedef enum { BOOT_SAFE, CONFIGURING, DISARMED, RUNNING, FAULT_LATCHED, CLEAR_CHECK } ProtectionState;
typedef struct {
    /* reason可能在后续检查时更新；first_fault只保存首次锁存快照，
     * boot保存SDK初始化清EIF之前的快照，不应混为同一次采样。 */
    ProtectionResult reason;
    ProtectionSnapshot boot, first_fault;
    uint32_t latched;
} ProtectionFaultInfo;
/* All calls serialize through platform critical sections. No asynchronous RUN queue. */
/* 仅BOOT_SAFE允许初始化；重复调用不会清除软件锁存。 */
ProtectionResult Protection_Init(void);
/* 仅DISARMED且RAW/EIF/配置/锁存全部允许时才请求平台开启MOE。 */
ProtectionResult Protection_RequestRun(void);
/* 请求安全关断；不能清锁存，返回值还可能报告已存在的阻塞或故障。 */
ProtectionResult Protection_RequestStop(void);
/* 只在锁存态、有效RAW已撤销且MOE=0时尝试清除；成功回DISARMED。
 * CLEAR永不调用MOE开启。当前RAW无效时返回INVALID并保持原状态。 */
ProtectionResult Protection_RequestClear(void);
/* 周期监视只检查/锁存，不自动清除或重新RUN；不能保证软件响应时限。 */
ProtectionResult Protection_Process(void);
ProtectionState Protection_GetState(void);
/* 在临界区复制记录；out为NULL时不写。 */
void Protection_GetFaultInfo(ProtectionFaultInfo *out);
#endif
