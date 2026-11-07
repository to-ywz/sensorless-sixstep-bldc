/**
 * @file test_mcpwm_complementary.c
 * @brief A2: one complementary MCPWM pair, logic-only bench verification.
 *
 * Target: LKS32MC071CBT8.
 * CH0P -> P1.4 (UP), CH0N -> P1.5 (UN), both AF3_MCPWM.
 * START1 -> P2.11 active-low; STOP1 -> P2.12 active-low.
 *
 * SAFETY:
 * - Disconnect the inverter DC bus.
 * - Disconnect FN550-90K and all motor phases.
 * - Observe only MCU logic pins P1.4/P1.5 relative to MCU logic GND.
 * - This is not a gate-driver, power-stage, or hardware protection test.
 */
#include <stdint.h>
#include "lks32mc07x_lib.h"
#include "test_mcpwm_complementary.h"

#define BENCH_MCPWM_PERIOD_COUNTS       2400u
#define BENCH_MCPWM_EDGE_COUNTS         600
#define BENCH_MCPWM_DEADTIME_COUNTS      48u  /* approx. 0.99 us at measured A1 clock */
#define BENCH_BUTTON_START_PIN          GPIO_Pin_11
#define BENCH_BUTTON_STOP_PIN           GPIO_Pin_12
#define BENCH_BUTTON_DEBOUNCE_SAMPLES   20u

typedef struct
{
    uint8_t candidate_pressed;
    uint8_t stable_pressed;
    uint8_t stable_count;
    uint8_t press_event;
} BenchButton_t;

static BenchButton_t g_start_button;
static uint8_t g_pwm_enabled;

static uint8_t bench_button_read_pressed(uint16_t pin)
{
    return (GPIO_ReadInputDataBit(GPIO2, pin) == 0u) ? 1u : 0u;
}

static void bench_button_init(BenchButton_t *button, uint8_t pressed)
{
    button->candidate_pressed = pressed;
    button->stable_pressed = pressed;
    button->stable_count = BENCH_BUTTON_DEBOUNCE_SAMPLES;
    button->press_event = 0u;
}

/** Return one event only after a stable released-to-pressed transition. */
static uint8_t bench_button_update(BenchButton_t *button, uint8_t pressed)
{
    button->press_event = 0u;

    if (pressed != button->candidate_pressed)
    {
        button->candidate_pressed = pressed;
        button->stable_count = 0u;
        return 0u;
    }

    if (button->stable_count < BENCH_BUTTON_DEBOUNCE_SAMPLES)
    {
        button->stable_count++;
    }

    if ((button->stable_count >= BENCH_BUTTON_DEBOUNCE_SAMPLES) &&
        (button->stable_pressed != button->candidate_pressed))
    {
        button->stable_pressed = button->candidate_pressed;
        if (button->stable_pressed != 0u)
        {
            button->press_event = 1u;
        }
    }

    return button->press_event;
}

void bench_mcpwm_complementary_init(void)
{
    MCPWM_InitTypeDef cfg;
    GPIO_InitTypeDef gpio;

    /* START1/STOP1 are external 5V pull-up, active-low inputs. */
    GPIO_PinAFConfig(GPIO2, GPIO_PinSource_11, AF0_GPIO);
    GPIO_PinAFConfig(GPIO2, GPIO_PinSource_12, AF0_GPIO);

    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = BENCH_BUTTON_START_PIN | BENCH_BUTTON_STOP_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IN;
    gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIO2, &gpio);

    bench_button_init(&g_start_button,
                      bench_button_read_pressed(BENCH_BUTTON_START_PIN));
    g_pwm_enabled = 0u;

    /* board_init_safe() has already forced P1.4..P1.9 to GPIO-low. */
    MCPWM_StructInit(&cfg);

    cfg.MCLK_EN = ENABLE;
    cfg.CLK_DIV = 0u;
    cfg.BASE_CNT0_EN = ENABLE;

    cfg.TH0 = BENCH_MCPWM_PERIOD_COUNTS;
    cfg.TH00 = (uint16_t)(-BENCH_MCPWM_EDGE_COUNTS);
    cfg.TH01 = (uint16_t)( BENCH_MCPWM_EDGE_COUNTS);

    /*
     * 0 = center-aligned mode. With IO0_PPE disabled and CH0 P/N both
     * hardware-controlled, this is the normal complementary pair mode.
     */
    cfg.MCPWM_WorkModeCH0 = MCPWM0_CENTRAL_PWM_MODE;
    cfg.IO0_PPE = DISABLE;

    /*
     * Both outputs are timer-controlled. In A1 CH0_NS was set to 1
     * to force CH0N low; A2 deliberately leaves CH0_NS=0.
     */
    cfg.CH0_PS = DISABLE;
    cfg.CH0_NS = DISABLE;

    cfg.CH0P_Polarity_INV = DISABLE;
    cfg.CH0N_Polarity_INV = DISABLE;
    cfg.Switch_CH0N_CH0P = DISABLE;

    /*
     * DTH00/DTH01 are shared by CH0/1/2 in the current SDK structure.
     * Equal values avoid any P/N direction ambiguity in this first test.
     */
    cfg.DeadTimeCH012N = BENCH_MCPWM_DEADTIME_COUNTS;
    cfg.DeadTimeCH012P = BENCH_MCPWM_DEADTIME_COUNTS;

    cfg.CH0P_default_output = MCPWM0_LOW_LEVEL;
    cfg.CH0N_default_output = MCPWM0_LOW_LEVEL;

    /* Keep the other four phase outputs statically low and unmuxed. */
    cfg.CH1_PS = ENABLE;
    cfg.CH1P_SCTRLP = MCPWM0_LOW_LEVEL;
    cfg.CH1_NS = ENABLE;
    cfg.CH1N_SCTRLN = MCPWM0_LOW_LEVEL;

    cfg.CH2_PS = ENABLE;
    cfg.CH2P_SCTRLP = MCPWM0_LOW_LEVEL;
    cfg.CH2_NS = ENABLE;
    cfg.CH2N_SCTRLN = MCPWM0_LOW_LEVEL;

    cfg.CH1P_default_output = MCPWM0_LOW_LEVEL;
    cfg.CH1N_default_output = MCPWM0_LOW_LEVEL;
    cfg.CH2P_default_output = MCPWM0_LOW_LEVEL;
    cfg.CH2N_default_output = MCPWM0_LOW_LEVEL;

    cfg.TR0_T0_UpdateEN = ENABLE;
    cfg.TR0_AEC = DISABLE;

    MCPWM_Init(&cfg);
    PWMOutputs(DISABLE);

    /* Keep the measured pair in GPIO-low mode until START1 is pressed. */
    GPIO_PinAFConfig(GPIO1, GPIO_PinSource_4, AF0_GPIO);
    GPIO_PinAFConfig(GPIO1, GPIO_PinSource_5, AF0_GPIO);
    GPIO_ResetBits(GPIO1, GPIO_Pin_4 | GPIO_Pin_5);
}

void bench_mcpwm_complementary_enable(void)
{
    if (g_pwm_enabled != 0u)
    {
        return;
    }

    /* Both pins are muxed only after the MCPWM outputs are disabled. */
    PWMOutputs(DISABLE);
    GPIO_PinAFConfig(GPIO1, GPIO_PinSource_4, AF3_MCPWM);
    GPIO_PinAFConfig(GPIO1, GPIO_PinSource_5, AF3_MCPWM);
    PWMOutputs(ENABLE);
    g_pwm_enabled = 1u;
}

void bench_mcpwm_complementary_disable(void)
{
    /* Remove MCPWM output enable first; CH0P/CH0N default state is low. */
    PWMOutputs(DISABLE);
    GPIO_PinAFConfig(GPIO1, GPIO_PinSource_4, AF0_GPIO);
    GPIO_PinAFConfig(GPIO1, GPIO_PinSource_5, AF0_GPIO);
    GPIO_ResetBits(GPIO1, GPIO_Pin_4 | GPIO_Pin_5);
    g_pwm_enabled = 0u;
}

void bench_mcpwm_complementary_poll(void)
{
    uint8_t stop_pressed;
    uint8_t start_pressed;
    uint8_t start_event;

    stop_pressed = bench_button_read_pressed(BENCH_BUTTON_STOP_PIN);
    start_pressed = bench_button_read_pressed(BENCH_BUTTON_START_PIN);
    start_event = bench_button_update(&g_start_button, start_pressed);

    /* STOP is immediate and always wins over a simultaneous START event. */
    if (stop_pressed != 0u)
    {
        if (g_pwm_enabled != 0u)
        {
            bench_mcpwm_complementary_disable();
        }
        return;
    }

    if (start_event != 0u)
    {
        bench_mcpwm_complementary_enable();
    }
}
