/**
 * @file board_init.c
 * @brief V0.1-A0 MCU071 board bring-up.
 *
 * Schematic: LKS_EVB_MCU071CBT8_V1.0.
 * Six gate input signals are on P1.4..P1.9.
 * P0.6 is the LED1/test net on the MCU control board.
 *
 * GPIO low is a TEST CONDITION, NOT PROVEN POWER-STAGE SAFETY.
 * Do not power the inverter until LKS560 inputs, startup states,
 * and MCPWM dead-time are verified separately.
 */
#include "lks32mc07x_lib.h"
#include "board_init.h"

#define BOARD_PWM_IO_MASK   (GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | \
                             GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9)
#define BOARD_HEARTBEAT_IO  (GPIO_Pin_6)

static void board_force_pwm_pads_low(void)
{
    GPIO_InitTypeDef gpio;
    unsigned int pin;

    SYS_ModuleClockCmd(SYS_Module_GPIO, ENABLE);

    /*
     * Preload GPIO output latches before making pads outputs.
     * Do not select AF3_MCPWM in the A0 test.
     */
    for (pin = 4u; pin <= 9u; ++pin) {
        GPIO_PinAFConfig(GPIO1, pin, AF0_GPIO);
    }

    GPIO_ResetBits(GPIO1, BOARD_PWM_IO_MASK);

    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = BOARD_PWM_IO_MASK;
    gpio.GPIO_Mode = GPIO_Mode_OUT;
    gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIO1, &gpio);
    GPIO_ResetBits(GPIO1, BOARD_PWM_IO_MASK);
}

static void board_init_heartbeat(void)
{
    GPIO_InitTypeDef gpio;

    GPIO_PinAFConfig(GPIO0, GPIO_PinSource_6, AF0_GPIO);
    GPIO_ResetBits(GPIO0, BOARD_HEARTBEAT_IO);

    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = BOARD_HEARTBEAT_IO;
    gpio.GPIO_Mode = GPIO_Mode_OUT;
    gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIO0, &gpio);
}

void board_init_safe(void)
{
    __disable_irq();

    board_force_pwm_pads_low();

    /* Vendor SDK V3.16, internal oscillator: 48 MHz for A0 testing. */
    SYS_MclkChoice(SYS_MCLK_48M_RC);

    /* Bench-only policy: no unexpected watchdog reset in A0. */
    IWDG_DISABLE();

    board_init_heartbeat();
    __enable_irq();
}

void board_heartbeat_toggle(void)
{
    if (GPIO_ReadOutputDataBit(GPIO0, BOARD_HEARTBEAT_IO) != 0u) {
        GPIO_ResetBits(GPIO0, BOARD_HEARTBEAT_IO);
    } else {
        GPIO_SetBits(GPIO0, BOARD_HEARTBEAT_IO);
    }
}
