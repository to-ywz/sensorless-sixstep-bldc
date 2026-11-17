/**
 * @file protection_manager.c
 * @brief 严重故障优先锁存；RUN、STOP、CLEAR互不隐式替代。
 * 任何失败均保留关闭请求；硬件FAIL负责首先异步关断，软件负责禁止重开。
 * 临界区串行化可屏蔽中断与主循环请求，不能消除硬件故障检查与置MOE竞态。
 */
#include "protection_manager.h"
static ProtectionState state=BOOT_SAFE;
static ProtectionFaultInfo fault;
#define FAIL1_FLAG (1u << 5)
/* 先尝试关输出，再锁存；关断失败优先作为返回原因。
 * 首次现场只存一次，后续轮询不得覆盖first_fault。调用方持有临界区。 */
static ProtectionResult latch(ProtectionResult reason, const ProtectionSnapshot *s)
{
    ProtectionResult off=LksProtection_SetMoe(0u);
    if (!fault.latched) fault.first_fault=*s;
    fault.latched=1u;
    fault.reason=(off == PROTECTION_OK) ? reason : off;
    state=FAULT_LATCHED;
    return fault.reason;
}
/* 检查顺序：有效故障证据 > 配置/读数资格阻塞 > MOE状态异常。
 * 无效RAW的占位0不能被解释为故障已撤销。
 * 函数可能关断并改变状态，不是无副作用的查询接口。 */
static ProtectionResult inspect(ProtectionSnapshot *s)
{
    LksProtection_Read(s);
    if ((s->raw_valid && s->raw) || (s->mcpwm_readable && (s->eif & FAIL1_FLAG))) return latch(PROTECTION_FAULT,s); // 故障读取
    if (!s->mcpwm_readable || !s->raw_valid || !s->config_valid || s->blockers) return latch(PROTECTION_BLOCKED,s); // 阻塞读取
    if ((state == RUNNING && !s->moe) || (state != RUNNING && s->moe))
        return latch(PROTECTION_IO_ERROR,s);    // MOE异常
    return PROTECTION_OK;
}
ProtectionResult Protection_Init(void)
{
    ProtectionResult r;
    ProtectionSnapshot s;
    uint32_t key=LksProtection_EnterCritical();
    /* Reinitialization must never erase a software fault latch. */
    if (state != BOOT_SAFE) { LksProtection_ExitCritical(key); return PROTECTION_INVALID; }
    /* 即使SDK清除了启动EIF，原boot证据仍可使初始化进入锁存态。 */
    state=CONFIGURING;
    r=LksProtection_Init(&fault.boot);
    LksProtection_Read(&s);
    if (fault.boot.mcpwm_readable && (fault.boot.eif & FAIL1_FLAG)) r=PROTECTION_FAULT;
    if (r != PROTECTION_OK) r=latch(r,&s);
    else { r=inspect(&s); if (r == PROTECTION_OK) state=DISARMED; }
    LksProtection_ExitCritical(key);
    return r;
}
ProtectionResult Protection_RequestRun(void)
{
    ProtectionSnapshot s;
    ProtectionResult r;
    uint32_t key=LksProtection_EnterCritical();
    r=inspect(&s);
    if (r == PROTECTION_OK) {
        if (state != DISARMED || fault.latched) r=PROTECTION_INVALID;
        else {
            /* 当前平台直接BLOCKED；未来放行后仍需再次检查。
             * 后检查不能证明不存在硬件重开窗口，资格门禁不能跳过。 */
            r=LksProtection_SetMoe(1u);
            if (r == PROTECTION_OK) { state=RUNNING; r=inspect(&s); }
            else r=latch(r,&s);
        }
    }
    LksProtection_ExitCritical(key);
    return r;
}
ProtectionResult Protection_RequestStop(void)
{
    ProtectionSnapshot s;
    ProtectionResult r;
    uint32_t key=LksProtection_EnterCritical();
    /* 先收集故障再STOP，避免普通停止掩盖已存在的锁存条件。 */
    r=inspect(&s);
    if (LksProtection_SetMoe(0u) != PROTECTION_OK) r=latch(PROTECTION_IO_ERROR,&s);
    if (r == PROTECTION_OK && !fault.latched && state == RUNNING) state=DISARMED;
    LksProtection_ExitCritical(key);
    return r;
}
ProtectionResult Protection_RequestClear(void)
{
    ProtectionSnapshot s;
    ProtectionResult r=PROTECTION_INVALID;
    uint32_t key=LksProtection_EnterCritical();
    LksProtection_Read(&s);
    if (state == FAULT_LATCHED && s.mcpwm_readable && s.raw_valid && !s.raw && !s.moe) {
        /* 已确认读数有效且源撤销，才允许进入清除阶段。
         * 平台清EIF后必须重读；重新置位或资格不满足均返回锁存。 */
        state=CLEAR_CHECK;
        r=LksProtection_ClearFail();
        if (r == PROTECTION_OK) r=inspect(&s);
        if (r == PROTECTION_OK) { fault.latched=0u; state=DISARMED; }
        else r=latch(r,&s);
    }
    LksProtection_ExitCritical(key);
    return r;
}
ProtectionResult Protection_Process(void)
{
    ProtectionSnapshot s;
    ProtectionResult r;
    uint32_t key=LksProtection_EnterCritical();
    r=inspect(&s);
    /* 源撤销也不能靠普通周期检查清除锁存或恢复输出。 */
    if (fault.latched && r == PROTECTION_OK) r=fault.reason;
    LksProtection_ExitCritical(key);
    return r;
}
ProtectionState Protection_GetState(void)
{
    uint32_t key=LksProtection_EnterCritical();
    ProtectionState s=state;
    LksProtection_ExitCritical(key);
    return s;
}
void Protection_GetFaultInfo(ProtectionFaultInfo *out)
{
    uint32_t key=LksProtection_EnterCritical();
    if (out != 0) *out=fault;
    LksProtection_ExitCritical(key);
}
