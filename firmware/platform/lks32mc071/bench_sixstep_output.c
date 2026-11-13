#include "lks32mc07x_lib.h"
#include "board_pwm_pins.h"
#include "board_init.h"
#include "lks560_pwm_defaults.h"
#include "sixstep_table.h"
#include "bench_sixstep_output.h"

/* Nominal RC clock; adjust only from a measured clock baseline.
 * SysTick is dedicated to A4, polled, no interrupt. Poll at least every 349 ms.
 */
#ifndef BENCH_A4_CLOCK_HZ
#define BENCH_A4_CLOCK_HZ 48000000u
#endif
#define PERIOD 2400u
#define TICK_MASK 0x00ffffffu
/* Register basis: SDK V3.16 Include/lks32mc07x_user_manual.h and
 * user_manual/13_MCPWM.md, IO01/IO23, UPDATE, TCLK, FAIL012 and PRT.
 * Each channel occupies one byte; CH1/CH3 use the upper byte.
 * N inversion adapts the driver only AFTER internal active-high requests;
 * swap and work-mode bits remain zero.
 */
#define IO_CHANNEL_SHIFT       8u
#define IO_UPPER_CHANNEL_MASK  (0xffu << IO_CHANNEL_SHIFT)
/* Center-aligned IO retains internal conflict protection even for software
 * output (manual 13_MCPWM, IO control). Keep internal P/N requests mutually
 * exclusive, then invert N at the pad for LKS560's active-low LIN.
 * Bench: CH0=0x05 -> UP PWM (~24%), UN high. CH_DEF bypasses inversion.
 */
#define IO_SOFTWARE_PAIR       (MCPWM0_IO01_CH0_NS_BIT | MCPWM0_IO01_CH0_PS_BIT)
#define IO_DRIVER_N_INVERT     MCPWM0_IO01_CH0_NP_BIT
#define IO_PAIR_OFF            (IO_SOFTWARE_PAIR | IO_DRIVER_N_INVERT)
#define IO_HIGH_SIDE_PWM       (MCPWM0_IO01_CH0_NS_BIT | IO_DRIVER_N_INVERT)
#define IO_LOW_SIDE_ON         (IO_PAIR_OFF | MCPWM0_IO01_CH0_SCTRLN_BIT)
#define UPDATE_CH012_THRESHOLDS (MCPWM0_UPDATE_TH00_UPDATE_BIT | \
                                MCPWM0_UPDATE_TH01_UPDATE_BIT | \
                                MCPWM0_UPDATE_TH10_UPDATE_BIT | \
                                MCPWM0_UPDATE_TH11_UPDATE_BIT | \
                                MCPWM0_UPDATE_TH20_UPDATE_BIT | \
                                MCPWM0_UPDATE_TH21_UPDATE_BIT)
#define UPDATE_CH012_AND_CNT0 (UPDATE_CH012_THRESHOLDS | MCPWM0_UPDATE_CNT0_UPDATE_BIT)
#define MCPWM_WRITE_UNLOCK 0xDEADu
#define MCPWM_WRITE_LOCK   0xCAFEu
static uint32_t last_tick, elapsed_ticks;

uint32_t bench_sixstep_clock(void)
{
    uint32_t now = SysTick->VAL;
    elapsed_ticks += (last_tick - now) & TICK_MASK;
    last_tick = now;
    return elapsed_ticks;
}
uint32_t bench_sixstep_ticks_per_ms(void) { return BENCH_A4_CLOCK_HZ / 1000u; }
static void blank_delay(void)
{
    uint32_t start = bench_sixstep_clock();
    /* 5 nominal us per guard, deliberately longer than proposed 2 us minimum. */
    while ((uint32_t)(bench_sixstep_clock()-start) < BENCH_A4_CLOCK_HZ/200000u) {}
}
void bench_sixstep_output_stop(void)
{
    /* CH_DEF first commands HIN=0/LIN=1 together; then GPIO preserves it. */
    PWMOutputs(DISABLE);
    board_pwm_inputs_off();
    __DSB();
}
void bench_sixstep_output_init(void)
{
    MCPWM_InitTypeDef cfg;
    /* board_init_safe() established HIN=0/LIN=1 before driver setup. */
    SysTick->CTRL = 0u;
    SysTick->LOAD = TICK_MASK;
    SysTick->VAL = 0u;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
    while (SysTick->VAL == 0u) {}
    last_tick = SysTick->VAL;
    elapsed_ticks = 0u;
    MCPWM_StructInit(&cfg);
    cfg.MCLK_EN = ENABLE;
    cfg.CLK_DIV = 0u;
    cfg.BASE_CNT0_EN = ENABLE;
    cfg.TH0 = PERIOD;
    cfg.TH00 = cfg.TH10 = cfg.TH20 = (uint16_t)(-600);
    cfg.TH01 = cfg.TH11 = cfg.TH21 = 600u;
    cfg.MCPWM_WorkModeCH0 = cfg.MCPWM_WorkModeCH1 =
        cfg.MCPWM_WorkModeCH2 = MCPWM0_CENTRAL_PWM_MODE;
    cfg.IO0_PPE = cfg.IO1_PPE = cfg.IO2_PPE = DISABLE;
    cfg.CH0_PS = cfg.CH0_NS = cfg.CH1_PS = cfg.CH1_NS =
        cfg.CH2_PS = cfg.CH2_NS = ENABLE;
    cfg.CH0P_SCTRLP = cfg.CH1P_SCTRLP = cfg.CH2P_SCTRLP = MCPWM0_LOW_LEVEL;
    cfg.CH0N_SCTRLN = cfg.CH1N_SCTRLN = cfg.CH2N_SCTRLN = MCPWM0_LOW_LEVEL;
    cfg.CH0P_Polarity_INV = cfg.CH1P_Polarity_INV = cfg.CH2P_Polarity_INV = DISABLE;
    cfg.CH0N_Polarity_INV = cfg.CH1N_Polarity_INV = cfg.CH2N_Polarity_INV = ENABLE;
    lks560_pwm_set_off_defaults(&cfg);
    cfg.DeadTimeCH012N = cfg.DeadTimeCH012P = 48u;
    cfg.TR0_AEC = DISABLE;
    cfg.TR0_T0_UpdateEN = DISABLE;
    cfg.AUEN = 0u;
    MCPWM_Init(&cfg);
    bench_sixstep_output_stop();
}
static int apply_requests(uint32_t pwm_phase, uint32_t sink_phase, uint32_t duty_permille)
{
    uint32_t pin, i, io[3], width;
    int32_t half;
    /* Invalid inputs also remove the previous valid output immediately. */
    bench_sixstep_output_stop();
    if ((pwm_phase >= 3u && pwm_phase != 255u) ||
        (sink_phase >= 3u && sink_phase != 255u) ||
        pwm_phase == sink_phase || !sixstep_duty_valid(duty_permille)) return 0;
    blank_delay();
    /* SDK IO01/IO23 layout: NS bit2, PS bit3, N static level bit4.
     * N is inverted, no P/N exchange or push-pull. IO fields are not in UPDATE mask;
     * writes occur with MOE cleared AND all measured pads held HIN=0/LIN=1.
     */
    for (i=0u;i<3u;++i) io[i]=IO_PAIR_OFF; /* internal P/N=0/0 -> physical HIN/LIN=0/1 */
    if (pwm_phase < 3u) io[pwm_phase]=IO_HIGH_SIDE_PWM;       /* internal P=timer, N=0 -> LIN high */
    if (sink_phase < 3u) io[sink_phase]=IO_LOW_SIDE_ON;      /* internal P=0, N=1 -> LIN low */
    width=((2u*PERIOD+1u)*duty_permille+500u)/1000u;
    half=(int32_t)(width/2u);
    MCPWM0_PRT=MCPWM_WRITE_UNLOCK;
    /* Pause counter0 while loading a complete state; this bench explicitly
     * restarts PWM phase at each commutation, not production synchronous PWM.
     */
    MCPWM0_TCLK &= ~MCPWM0_TCLK_BASE_CNT0_EN_BIT;
    MCPWM0_IO01=io[0] | (io[1]<<IO_CHANNEL_SHIFT);
    MCPWM0_IO23=(MCPWM0_IO23 & IO_UPPER_CHANNEL_MASK) | io[2];
    MCPWM0_TH00=MCPWM0_TH10=MCPWM0_TH20=(uint16_t)(-half);
    MCPWM0_TH01=MCPWM0_TH11=MCPWM0_TH21=(uint16_t)((int32_t)width-half);
    MCPWM0_CNT0=(uint16_t)(-(int32_t)PERIOD);
    /* Only compare thresholds and CNT0: do not write reserved update bits. */
    MCPWM0_UPDATE=UPDATE_CH012_AND_CNT0;
    __DSB();
    MCPWM0_PRT=MCPWM_WRITE_LOCK;
    blank_delay();
    for (pin=BOARD_PWM_FIRST_PIN;pin<=BOARD_PWM_LAST_PIN;++pin)
        GPIO_PinAFConfig(GPIO1,pin,AF3_MCPWM);
    /* Check physical STOP again immediately before enabling. */
    if (GPIO_ReadInputDataBit(GPIO2,GPIO_Pin_12)==0u) {
        bench_sixstep_output_stop(); return 0;
    }
    MCPWM0_PRT=MCPWM_WRITE_UNLOCK;
    MCPWM0_TCLK |= MCPWM0_TCLK_BASE_CNT0_EN_BIT;
    MCPWM0_FAIL012 |= MCPWM0_FAIL012_MOE_BIT; /* CH0/1/2 only; do not enable CH3/4/5. */
    MCPWM0_PRT=MCPWM_WRITE_LOCK;
    return 1;
}

int bench_sixstep_output_apply(uint32_t sector, uint32_t duty_permille)
{
    SixstepState state;
    if (!sixstep_get(sector, &state)) {
        bench_sixstep_output_stop();
        return 0;
    }
    return apply_requests(state.pwm_phase, state.sink_phase, duty_permille);
}

int bench_gate_output_apply(uint32_t phase, uint32_t high_pwm)
{
    if (phase >= 3u || high_pwm > 1u) {
        bench_sixstep_output_stop();
        return 0;
    }
    return apply_requests(high_pwm ? phase : 255u, high_pwm ? 255u : phase, 250u);
}
