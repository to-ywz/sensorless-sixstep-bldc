/**
 * @file test_mcpwm_single.c
 * @brief A1: single MCPWM channel bench verification (logic only).
 *
 * Hardware: LKS32MC071CBT8 + LKS_EVB_MVPOWPRE_V5.0.
 * Motor and inverter DC bus MUST remain disconnected.
 * Only P1.4 (CH0P / UP) is switched to MCPWM alternate function.
 * Inactive pads remain at LKS560 OFF levels: HIN low, LIN high.
 *
 * Buttons from the power-board schematic:
 *   START1 -> MCU P2.11, active low, 10k pull-up to +5V, 100nF to GND.
 *   STOP1  -> MCU P2.12, active low, 10k pull-up to +5V, 100nF to GND.
 *
 * STOP1 is a software bench control, NOT a hardware emergency stop.
 * A1 does not validate dead-time, power-stage drive, or current protection.
 */
#include <stdint.h>
#include "lks32mc07x_lib.h"
#include "board_init.h"
#include "lks560_pwm_defaults.h"
#include "test_mcpwm_single.h"

/*
 * With SYS_MclkChoice(SYS_MCLK_48M_RC) in board_init_safe(),
 * a 2400-tick up/down carrier is expected to be approximately
 * 48 MHz / (2 * 2400) = 10 kHz. Verify on DHO914.
 */
#define BENCH_MCPWM_PERIOD_COUNTS       2400u
#define BENCH_MCPWM_EDGE_COUNTS         600
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

/**
 * @brief Update one active-low button state.
 * @return 1 only on a debounced released-to-pressed transition.
 */
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

void bench_mcpwm_single_init(void)
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

    /*
     * Do not generate a false START event if a key is held during reset:
     * initialize the debouncer from the actual pin state.
     */
    bench_button_init(&g_start_button,
                      bench_button_read_pressed(BENCH_BUTTON_START_PIN));
    g_pwm_enabled = 0u;

    /* board_init_safe() has established LKS560 OFF levels. */
    MCPWM_StructInit(&cfg);

    cfg.MCLK_EN = ENABLE;
    cfg.CLK_DIV = 0u;
    cfg.BASE_CNT0_EN = ENABLE;

    cfg.TH0 = BENCH_MCPWM_PERIOD_COUNTS;
    cfg.TH00 = (uint16_t)(-BENCH_MCPWM_EDGE_COUNTS);
    cfg.TH01 = (uint16_t)( BENCH_MCPWM_EDGE_COUNTS);

    cfg.MCPWM_WorkModeCH0 = MCPWM0_CENTRAL_PWM_MODE;
    cfg.TR0_T0_UpdateEN = ENABLE;

    /* Inactive internal requests are low; N pads are inverted to LIN high. */
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

    /* A1 observes CH0P only; disable P/N push-pull alternation. */
    cfg.IO0_PPE = DISABLE;

    /* No FAIL/BKIN and no power-stage protection is tested in A1. */
    cfg.TR0_AEC = DISABLE;

    /* N timer/software requests are active-high internally; invert only
     * the physical N pad for LKS560's active-low LIN. CH_DEF bypasses it.
     */
    cfg.CH0N_Polarity_INV = cfg.CH1N_Polarity_INV = cfg.CH2N_Polarity_INV = ENABLE;
    lks560_pwm_set_off_defaults(&cfg);
    MCPWM_Init(&cfg);
    PWMOutputs(DISABLE);

    /* Keep all driver inputs OFF until START1 is pressed. */
    board_pwm_inputs_off();
}

void bench_mcpwm_single_enable(void)
{
    if (g_pwm_enabled != 0u)
    {
        return;
    }

    /*
     * The inverter supply must remain physically disconnected.
     * During mux transition the pad might glitch; this is a measurement item.
     */
    GPIO_PinAFConfig(GPIO1, GPIO_PinSource_4, AF3_MCPWM);
    PWMOutputs(ENABLE);
    g_pwm_enabled = 1u;
}

void bench_mcpwm_single_disable(void)
{
    /* First clear MOE, then preserve physical HIN=0/LIN=1 in GPIO mode. */
    PWMOutputs(DISABLE);
    board_pwm_inputs_off();
    g_pwm_enabled = 0u;
}

void bench_mcpwm_single_poll(void)
{
    uint8_t stop_pressed;
    uint8_t start_pressed;
    uint8_t start_event;

    /* STOP is deliberately acted on before any START request. */
    stop_pressed = bench_button_read_pressed(BENCH_BUTTON_STOP_PIN);
    start_pressed = bench_button_read_pressed(BENCH_BUTTON_START_PIN);

    /* Update START even while STOP is held, preventing delayed auto-start. */
    start_event = bench_button_update(&g_start_button, start_pressed);

    if (stop_pressed != 0u)
    {
        if (g_pwm_enabled != 0u)
        {
            bench_mcpwm_single_disable();
        }
        return;
    }

    if (start_event != 0u)
    {
        bench_mcpwm_single_enable();
    }
}
