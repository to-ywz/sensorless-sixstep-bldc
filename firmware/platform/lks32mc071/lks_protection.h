/**
 * @file lks_protection.h
 * @brief LKS32MC071保护寄存器适配接口，不决定电机运行权限。
 * 调用关系：Bench -> Protection Manager -> 本模块 -> SDK/寄存器。
 * 当前为部分实现：模拟输入、滤波入口编号及恢复资格未闭环，禁止开启MOE。
 * 快照中的有效性标志必须先于数值使用；占位零值不代表硬件正常。
 */
#ifndef LKS_PROTECTION_H
#define LKS_PROTECTION_H
#include <stdint.h>
/* OK=操作完成；BLOCKED=缺少资格证据；FAULT=观测到故障；
 * INVALID=参数或调用状态不合法；IO_ERROR=硬件写入/读回不符合要求。
 * BLOCKED与IO_ERROR不同：前者不能靠重试RUN消除。 */
typedef enum { PROTECTION_OK, PROTECTION_BLOCKED, PROTECTION_FAULT, PROTECTION_INVALID, PROTECTION_IO_ERROR } ProtectionResult;
/* blockers为位集合：入口时钟编号、窗口直通、模拟输入和重启机制。
 * CLOCK不表示bit10未知：bit10已确认，未闭环的是滤波入口网络编号。 */
#define PROTECTION_BLOCK_CLOCK 1u
#define PROTECTION_BLOCK_WINDOW 2u
#define PROTECTION_BLOCK_ANALOG 4u
#define PROTECTION_BLOCK_RESTART 8u
typedef struct {
    uint32_t fail012, eif, ch_def, ch_mask, tclk, flt, sdcfg, auen;
    /* raw仅raw_valid=1时可解释；moe仅mcpwm_readable=1时有效。
     * config_valid表示可供Manager使用的完整配置资格，当前恒0。 */
    uint32_t raw, moe, config_valid, blockers;
    /* Zero validity means corresponding values must not be consumed. */
    uint32_t sys_clk_fen, mcpwm_readable, raw_valid;
    /* safe_config_matches只核对关闭态配置；hardware_qualified表示硬件资格。
     * 关闭态读回正确不等于FAIL链已使能，更不等于实机测试通过。 */
    uint32_t safe_config_matches, hardware_qualified;
} ProtectionSnapshot;
/* 初始化前由board_init_safe建立安全GPIO。boot必填，保存SDK清EIF前证据。
 * 返回BLOCKED是当前预期；IO_ERROR表示连关闭态配置也未正确建立。 */
ProtectionResult LksProtection_Init(ProtectionSnapshot *boot);
/* Before initialization: returns clock state, no MCPWM/CMP accesses.
 * RAW stays invalid until qualified analog setup exists. NULL is ignored. */
void LksProtection_Read(ProtectionSnapshot *out);
/* Disable before initialization enables/verifies module clock first.
 * Enable remains BLOCKED in this unqualified build. */
ProtectionResult LksProtection_SetMoe(uint32_t enable);
/* 当前仅返回BLOCKED，尚未执行EIF写1清零。
 * 后续实现必须仅写FAIL1_IF位，不得读改写EIF，也不得顺带开启MOE。 */
ProtectionResult LksProtection_ClearFail(void);
/* Save/restore PRIMASK; manager APIs may run in foreground or interrupt context. */
uint32_t LksProtection_EnterCritical(void);
void LksProtection_ExitCritical(uint32_t key);
/* Diagnostic RAW is separate from the Manager qualification snapshot. */
uint32_t LksProtection_P0Init(void);
uint32_t LksProtection_P0ReadRaw(void);
uint32_t LksProtection_P1ProbeArm(void);
#ifdef M2_P1_DYNAMIC
uint32_t LksProtection_SelectBenchOutput(uint32_t output);
#endif
#endif
