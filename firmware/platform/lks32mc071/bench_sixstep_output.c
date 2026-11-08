#include "lks32mc07x_lib.h"
#include "board_pwm_pins.h"
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
    uint32_t pin;
    PWMOutputs(DISABLE);
    GPIO_ResetBits(GPIO1, BOARD_PWM_PIN_MASK);
    for (pin=BOARD_PWM_FIRST_PIN; pin<=BOARD_PWM_LAST_PIN; ++pin)
        GPIO_PinAFConfig(GPIO1, pin, AF0_GPIO);
    __DSB();
}
void bench_sixstep_output_init(void)
{
    MCPWM_InitTypeDef cfg;
    /* board_init_safe() has configured all six pads as GPIO outputs low. */
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
    cfg.CH0P_SCTRLP = cfg.CH0N_SCTRLN = cfg.CH1P_SCTRLP =
        cfg.CH1N_SCTRLN = cfg.CH2P_SCTRLP = cfg.CH2N_SCTRLN = MCPWM0_LOW_LEVEL;
    cfg.CH0P_default_output = cfg.CH0N_default_output =
        cfg.CH1P_default_output = cfg.CH1N_default_output =
        cfg.CH2P_default_output = cfg.CH2N_default_output = MCPWM0_LOW_LEVEL;
    cfg.CH0P_FAIL_EN = cfg.CH0N_FAIL_EN = cfg.CH1P_FAIL_EN =
        cfg.CH1N_FAIL_EN = cfg.CH2P_FAIL_EN = cfg.CH2N_FAIL_EN = ENABLE;
    cfg.DeadTimeCH012N = cfg.DeadTimeCH012P = 48u;
    cfg.TR0_AEC = DISABLE;
    cfg.TR0_T0_UpdateEN = DISABLE;
    cfg.AUEN = 0u;
    MCPWM_Init(&cfg);
    bench_sixstep_output_stop();
}
int bench_sixstep_output_apply(uint32_t sector, uint32_t duty_permille)
{
    SixstepState state;
    uint32_t pin, i, io[3], width;
    int32_t half;
    /* Invalid inputs also remove the previous valid output immediately. */
    bench_sixstep_output_stop();
    if (!sixstep_get(sector,&state) || !sixstep_duty_valid(duty_permille)) return 0;
    blank_delay();
    /* SDK IO01/IO23 layout: NS bit2, PS bit3, N static level bit4.
     * No inversion, exchange or push-pull. IO fields are not in UPDATE mask;
     * writes occur with MOE cleared AND all measured pads held GPIO-low.
     */
    for (i=0u;i<3u;++i) io[i]=0x0cu; /* P/N forced low */
    io[state.pwm_phase]=0x04u;       /* P timer, N forced low */
    io[state.sink_phase]=0x1cu;      /* P forced low, N forced high */
    width=((2u*PERIOD+1u)*duty_permille+500u)/1000u;
    half=(int32_t)(width/2u);
    MCPWM0_PRT=0xDEADu;
    /* Pause counter0 while loading a complete state; this bench explicitly
     * restarts PWM phase at each commutation, not production synchronous PWM.
     */
    MCPWM0_TCLK &= ~BIT6;
    MCPWM0_IO01=io[0] | (io[1]<<8);
    MCPWM0_IO23=(MCPWM0_IO23 & 0xff00u) | io[2];
    MCPWM0_TH00=MCPWM0_TH10=MCPWM0_TH20=(uint16_t)(-half);
    MCPWM0_TH01=MCPWM0_TH11=MCPWM0_TH21=(uint16_t)((int32_t)width-half);
    MCPWM0_CNT0=(uint16_t)(-(int32_t)PERIOD);
    /* Only compare thresholds and CNT0: do not write reserved update bits. */
    MCPWM0_UPDATE=0x083fu;
    __DSB();
    MCPWM0_PRT=0xCAFEu;
    blank_delay();
    for (pin=BOARD_PWM_FIRST_PIN;pin<=BOARD_PWM_LAST_PIN;++pin)
        GPIO_PinAFConfig(GPIO1,pin,AF3_MCPWM);
    /* Check physical STOP again immediately before enabling. */
    if (GPIO_ReadInputDataBit(GPIO2,GPIO_Pin_12)==0u) {
        bench_sixstep_output_stop(); return 0;
    }
    MCPWM0_PRT=0xDEADu;
    MCPWM0_TCLK |= BIT6;
    MCPWM0_FAIL012 |= BIT6; /* CH0/1/2 only; do not enable CH3/4/5. */
    MCPWM0_PRT=0xCAFEu;
    return 1;
}
