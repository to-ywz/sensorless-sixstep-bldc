/**
 * @file bench_cmp_fail.c
 * @brief 记录初始化、STOP及快照，不写MOE，不调用A5后端。
 * 上层main先执行board_init_safe。未来硬件注入须另行满足物理隔离门槛；
 * 当前入口没有DAC注入、PWM输出、延迟测量或完整P0-P4序列。
 */
#include "bench_cmp_fail.h"
#include "board_init.h"
#include "lks32mc07x_lib.h"
volatile CmpFailBenchReport bench_cmp_fail_report;
volatile uint32_t g_m2_p0_ready;
volatile uint32_t g_m2_p1_probe_command; /* One-shot arm request. */
volatile uint32_t g_m2_p1_probe_state; /* 0=idle, 1=armed, 2=rejected */
volatile uint32_t g_m2_p1_eif;
volatile uint32_t g_m2_p1_moe;
volatile uint32_t g_m2_p0_stimulus; /* 0=low, 1=high; other values drive low */
volatile uint32_t g_m2_p0_raw=2u; /* 2 = unavailable */
#ifdef M2_P1_DYNAMIC
volatile uint32_t g_m2_p2_output; /* 0..5 = UP, UN, VP, VN, WP, WN */
volatile uint32_t g_m2_p2_active_output;
volatile uint32_t g_m2_p1_command; /* 1=start once, 2=abort */
volatile uint32_t g_m2_p1_state; /* 0=blocked, 1=ready, 2=active, 3=ended, 4=aborted */
volatile ProtectionSnapshot g_m2_p1_before_fault, g_m2_p1_before_release,
                            g_m2_p1_before_stop, g_m2_p1_after_stop;
volatile ProtectionResult g_m2_p1_result;
volatile uint32_t g_m2_p1_inject_tick, g_m2_p1_release_tick, g_m2_p1_stop_tick;
static uint32_t start_tick, phase;
#define P1_TICK_MASK 0x00ffffffu
#define P1_PRE_TICKS 96000u
#define P1_PULSE_TICKS 24000u
#define P1_HOLD_TICKS 96000u
#define H1_TICKS_PER_US 48u
volatile uint32_t g_m2_h1_pulse_us=500u;
volatile uint32_t g_m2_h1_offset_us;
static uint32_t h1_pre_ticks, h1_pulse_ticks;

static void p1_finish(uint32_t state)
{
    ProtectionSnapshot s;
    LksProtection_Read(&s);
    g_m2_p1_before_stop=s;
    g_m2_p1_stop_tick=(start_tick-SysTick->VAL)&P1_TICK_MASK;
    g_m2_p1_result=Protection_RequestStop();
    board_p0_stimulus_set(0u);
    LksProtection_Read(&s);
    g_m2_p1_after_stop=s;
    g_m2_p1_state=state;
}

/* Recovery stages: rejected RUN, CLEAR, explicit RUN, final STOP. */
typedef struct {
    ProtectionResult result;
    ProtectionState manager_state;
    ProtectionSnapshot registers;
} M2RecoveryStep;
volatile uint32_t g_m2_recovery_command;
volatile uint32_t g_m2_recovery_state; /* 0=idle, 1=running, 2=pass, 3=fail */
volatile M2RecoveryStep g_m2_recovery_steps[4];
volatile ProtectionSnapshot g_m2_recovery_before_release, g_m2_recovery_before_stop;
static uint32_t recovery_tick, recovery_edge_tick, recovery_phase;

static ProtectionSnapshot recovery_record(uint32_t index, ProtectionResult result)
{
    M2RecoveryStep step;
    step.result=result;
    step.manager_state=Protection_GetState();
    LksProtection_Read(&step.registers);
    g_m2_recovery_steps[index]=step;
    return step.registers;
}

static void recovery_finish(uint32_t passed)
{
    ProtectionResult result=Protection_RequestStop();
    ProtectionSnapshot s=recovery_record(3u,result);
    board_p0_stimulus_set(0u);
    g_m2_recovery_state=(passed && result==PROTECTION_FAULT &&
        Protection_GetState()==FAULT_LATCHED &&
        s.mcpwm_readable && !s.moe) ? 2u : 3u;
}

static void recovery_poll(void)
{
    uint32_t command=g_m2_recovery_command;
    ProtectionResult result;
    ProtectionSnapshot s;
    g_m2_recovery_command=0u;
    if (g_m2_recovery_state==1u) {
        uint32_t elapsed=(recovery_tick-SysTick->VAL)&P1_TICK_MASK;
        LksProtection_Read(&s);
        if (command==2u || !s.raw_valid || !s.config_valid ||
            elapsed>=P1_PRE_TICKS+P1_PULSE_TICKS+P1_HOLD_TICKS+48000u) {
            recovery_finish(0u);
        } else if (recovery_phase==0u) {
            if (s.raw || !s.moe || (s.eif & MCPWM0_EIF_FAIL1_IF_BIT)) {
                recovery_finish(0u);
            } else if (elapsed>=P1_PRE_TICKS) {
                board_p0_stimulus_set(1u);
                recovery_edge_tick=(recovery_tick-SysTick->VAL)&P1_TICK_MASK;
                recovery_phase=1u;
            }
        } else if (recovery_phase==1u && elapsed-recovery_edge_tick>=P1_PULSE_TICKS) {
            g_m2_recovery_before_release=s;
            board_p0_stimulus_set(0u);
            recovery_edge_tick=(recovery_tick-SysTick->VAL)&P1_TICK_MASK;
            recovery_phase=2u;
        } else if (recovery_phase==2u && elapsed-recovery_edge_tick>=P1_HOLD_TICKS) {
            g_m2_recovery_before_stop=s;
            recovery_finish(g_m2_recovery_before_release.raw &&
                !g_m2_recovery_before_release.moe &&
                (g_m2_recovery_before_release.eif & MCPWM0_EIF_FAIL1_IF_BIT) &&
                !s.raw && !s.moe && (s.eif & MCPWM0_EIF_FAIL1_IF_BIT));
        }
        return;
    }
    if (command!=1u || g_m2_p1_state!=3u || g_m2_recovery_state!=0u) return;
    result=Protection_RequestRun();
    s=recovery_record(0u,result);
    if (result!=PROTECTION_FAULT || Protection_GetState()!=FAULT_LATCHED ||
        !s.raw_valid || s.raw || s.moe || !(s.eif & MCPWM0_EIF_FAIL1_IF_BIT)) {
        recovery_finish(0u);
        return;
    }
    result=Protection_RequestClear();
    s=recovery_record(1u,result);
    if (result!=PROTECTION_OK || Protection_GetState()!=DISARMED ||
        !s.raw_valid || s.raw || s.moe || (s.eif & MCPWM0_EIF_FAIL1_IF_BIT)) {
        recovery_finish(0u);
        return;
    }
    result=Protection_RequestRun();
    s=recovery_record(2u,result);
    if (result!=PROTECTION_OK || Protection_GetState()!=RUNNING || !s.moe) {
        recovery_finish(0u);
        return;
    }
    recovery_tick=SysTick->VAL;
    g_m2_recovery_state=1u;
}

/* Keep protection polling out of the short stimulus window. CMP1/FAIL1
 * remains active while CPU interrupts are masked. */
static void p1_emit_short_pulse(void)
{
    uint32_t key=LksProtection_EnterCritical();
    uint32_t rise_tick;
    uint32_t fall_tick;
    board_p0_stimulus_set(1u);
    rise_tick=SysTick->VAL;
    while (((rise_tick-SysTick->VAL)&P1_TICK_MASK)<h1_pulse_ticks) {
        /* Wait against the hardware counter for this bounded interval. */
    }
    board_p0_stimulus_set(0u);
    fall_tick=SysTick->VAL;
    LksProtection_ExitCritical(key);
    g_m2_p1_inject_tick=(start_tick-rise_tick)&P1_TICK_MASK;
    g_m2_p1_release_tick=(start_tick-fall_tick)&P1_TICK_MASK;
}

static void p1_poll(void)
{
    uint32_t command=g_m2_p1_command;
    ProtectionSnapshot s;
    if (command) g_m2_p1_command=0u;
    if (g_m2_p1_state==1u && command==1u) {
        uint32_t selected=g_m2_p2_output;
        uint32_t pulse_us=g_m2_h1_pulse_us;
        uint32_t offset_us=g_m2_h1_offset_us;
        if (pulse_us==0u || pulse_us>500u || offset_us>100u) {
            g_m2_p1_result=PROTECTION_INVALID;
            g_m2_p1_state=4u;
            return;
        }
        h1_pre_ticks=P1_PRE_TICKS+offset_us*H1_TICKS_PER_US;
        h1_pulse_ticks=pulse_us*H1_TICKS_PER_US;
        if (!LksProtection_SelectBenchOutput(selected)) {
            g_m2_p1_result=PROTECTION_INVALID;
            g_m2_p1_state=4u;
            return;
        }
        g_m2_p2_active_output=selected;
        g_m2_p1_result=Protection_RequestRun();
        if (g_m2_p1_result!=PROTECTION_OK) {
            g_m2_p1_state=4u;
            return;
        }
        start_tick=SysTick->VAL;
        g_m2_p1_state=2u;
    }
    if (g_m2_p1_state!=2u) return;
    uint32_t elapsed=(start_tick-SysTick->VAL)&P1_TICK_MASK;
    LksProtection_Read(&s);
    if (command==2u || !s.raw_valid || !s.config_valid ||
        elapsed>=h1_pre_ticks+h1_pulse_ticks+P1_HOLD_TICKS+48000u) {
        p1_finish(4u);
        return;
    }
    if (phase==0u) {
        if (s.raw || !s.moe || (s.eif & MCPWM0_EIF_FAIL1_IF_BIT)) {
            p1_finish(4u);
        } else if (elapsed>=h1_pre_ticks) {
            g_m2_p1_before_fault=s;
            if (h1_pulse_ticks<=50u*H1_TICKS_PER_US) {
                /* Short mode observes FAIL1 after release and before STOP. */
                p1_emit_short_pulse();
                phase=2u;
            } else {
                board_p0_stimulus_set(1u);
                g_m2_p1_inject_tick=(start_tick-SysTick->VAL)&P1_TICK_MASK;
                phase=1u;
            }
        }
    } else if (phase==1u && elapsed-g_m2_p1_inject_tick>=h1_pulse_ticks) {
        g_m2_p1_before_release=s;
        board_p0_stimulus_set(0u);
        g_m2_p1_release_tick=(start_tick-SysTick->VAL)&P1_TICK_MASK;
        phase=2u;
    } else if (phase==2u && elapsed-g_m2_p1_release_tick>=P1_HOLD_TICKS) {
        p1_finish(3u);
    }
}
#endif

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
#ifdef M2_P1_DYNAMIC
    if (bench_cmp_fail_report.init_result==PROTECTION_OK &&
        bench_cmp_fail_report.stop_result==PROTECTION_OK) {
        board_p0_stimulus_init();
        SysTick->LOAD=P1_TICK_MASK;
        SysTick->VAL=0u;
        SysTick->CTRL=SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
        g_m2_p1_state=1u;
    }
#endif
    LksProtection_Read(&s);
    Protection_GetFaultInfo(&f);
    bench_cmp_fail_report.registers=s;
    bench_cmp_fail_report.fault=f;
    /* Debugger-visible report. No injection, pin AF routing or RUN while blocked. */
    /* MOE读数为1时报告异常；读数无效时仍为BLOCKED而非PASS。
     * 此处没有测量六路pad，不可用MOE值替代物理默认态验收。 */
    if (s.moe) bench_cmp_fail_report.hardware_fail=BENCH_FAIL;
    /* Require confirmed disabled PWM before initializing the analog diagnostic. */
    if (bench_cmp_fail_report.init_result==PROTECTION_BLOCKED &&
        bench_cmp_fail_report.stop_result==PROTECTION_BLOCKED &&
        s.safe_config_matches)
        g_m2_p0_ready=LksProtection_P0Init();
    if (g_m2_p0_ready) board_p0_stimulus_init();
    bench_cmp_fail_report.completed=1u;
}
void bench_cmp_fail_poll(void)
{
#ifdef M2_P1_DYNAMIC
    p1_poll();
    recovery_poll();
    if (g_m2_p1_state!=2u && g_m2_recovery_state!=1u) (void)Protection_Process();
    return;
#endif
    (void)Protection_Process();
    if (g_m2_p0_ready) {
        uint32_t raw=LksProtection_P0ReadRaw();
        if (g_m2_p1_probe_command==1u) {
            g_m2_p1_probe_command=0u;
            if (g_m2_p1_probe_state==0u)
                g_m2_p1_probe_state=(g_m2_p0_stimulus==0u &&
                    LksProtection_P1ProbeArm()) ? 1u : 2u;
        }
        if (g_m2_p1_probe_state==1u) {
            ProtectionSnapshot s;
            LksProtection_Read(&s);
            g_m2_p1_eif=s.eif;
            g_m2_p1_moe=s.mcpwm_readable ? s.moe : 2u;
            if (!s.mcpwm_readable || s.moe) g_m2_p1_probe_state=2u;
        }
        board_p0_stimulus_set(raw != 2u && g_m2_p1_probe_state != 2u &&
                              g_m2_p0_stimulus == 1u);
        g_m2_p0_raw=raw;
    }
    /* This blocked bench never requests RUN; manager retains safe shutdown. */
}
