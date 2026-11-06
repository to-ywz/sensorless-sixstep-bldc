/**
 * @file test_mcpwm_single.c
 * @brief A1: single MCPWM channel bench verification (logic only).
 *
 * Hardware: LKS32MC071CBT8 control board, motor and inverter DC bus DISCONNECTED.
 * Only P1.4 (CH0P / UP) is switched to MCPWM alternate function.
 * P1.5-P1.9 remain GPIO outputs at low level from board_init_safe().
 *
 * NOTE: A1 does not validate dead-time, power-stage drive, or current protection.
 * Timing and duty are hypotheses until measured by an oscilloscope.
 */
#include "lks32mc07x_lib.h"
#include "test_mcpwm_single.h"

/*
 * With SYS_MclkChoice(SYS_MCLK_48M_RC) in board_init_safe(),
 * a 2400-tick up/down carrier is expected to be approximately
 * 48 MHz / (2 * 2400) = 10 kHz. Verify on DHO914.
 */
#define BENCH_MCPWM_PERIOD_COUNTS   2400u
#define BENCH_MCPWM_EDGE_COUNTS      600

void bench_mcpwm_single_init(void)
{
    MCPWM_InitTypeDef cfg;

    /* A0 GPIO low state must already be established. */
    MCPWM_StructInit(&cfg);

    cfg.MCLK_EN = ENABLE;
    cfg.CLK_DIV = 0u;
    cfg.BASE_CNT0_EN = ENABLE;

    cfg.TH0 = BENCH_MCPWM_PERIOD_COUNTS;
    cfg.TH00 = (uint16_t)(-BENCH_MCPWM_EDGE_COUNTS);
    cfg.TH01 = (uint16_t)( BENCH_MCPWM_EDGE_COUNTS);

    cfg.MCPWM_WorkModeCH0 = MCPWM0_CENTRAL_PWM_MODE;
    cfg.TR0_T0_UpdateEN = ENABLE;

    /* All inactive channels use the software-selected low state. */
    cfg.CH0_NS = 1u;
    cfg.CH0N_SCTRLN = MCPWM0_LOW_LEVEL;
    cfg.CH1_PS = 1u;
    cfg.CH1P_SCTRLP = MCPWM0_LOW_LEVEL;
    cfg.CH1_NS = 1u;
    cfg.CH1N_SCTRLN = MCPWM0_LOW_LEVEL;
    cfg.CH2_PS = 1u;
    cfg.CH2P_SCTRLP = MCPWM0_LOW_LEVEL;
    cfg.CH2_NS = 1u;
    cfg.CH2N_SCTRLN = MCPWM0_LOW_LEVEL;

    cfg.CH0P_default_output = MCPWM0_LOW_LEVEL;
    cfg.CH0N_default_output = MCPWM0_LOW_LEVEL;
    cfg.CH1P_default_output = MCPWM0_LOW_LEVEL;
    cfg.CH1N_default_output = MCPWM0_LOW_LEVEL;
    cfg.CH2P_default_output = MCPWM0_LOW_LEVEL;
    cfg.CH2N_default_output = MCPWM0_LOW_LEVEL;

    cfg.IO0_PPE = ENABLE;

    /* No FAIL/BKIN and no power-stage protection is tested in A1. */
    cfg.TR0_AEC = DISABLE;

    MCPWM_Init(&cfg);
    PWMOutputs(DISABLE);

    /* Keep P1.4 in GPIO-low mode until explicitly enabled below. */
    GPIO_PinAFConfig(GPIO1, GPIO_PinSource_4, AF0_GPIO);
    GPIO_ResetBits(GPIO1, GPIO_Pin_4);
}

void bench_mcpwm_single_enable(void)
{
    /*
     * The inverter supply must remain physically disconnected.
     * During mux transition the pad might glitch; this is a measurement item.
     */
    GPIO_PinAFConfig(GPIO1, GPIO_PinSource_4, AF3_MCPWM);
    PWMOutputs(ENABLE);
}

void bench_mcpwm_single_disable(void)
{
    /* First stop PWM hardware, then restore low GPIO output mode. */
    PWMOutputs(DISABLE);
    GPIO_PinAFConfig(GPIO1, GPIO_PinSource_4, AF0_GPIO);
    GPIO_ResetBits(GPIO1, GPIO_Pin_4);
}
