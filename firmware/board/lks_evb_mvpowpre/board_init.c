/**
 * @file board_init.c
 * @brief V0.1-A0 MCU071 board bring-up.
 *
 * Schematic: LKS_EVB_MCU071CBT8_V1.0.
 * Six gate input signals are on P1.4..P1.9.
 * P0.6 is the LED1/test net on the MCU control board.
 *
 * HIN-low/LIN-high is the LKS560 software OFF command.
 * Reset/high-impedance and actual VGS remain separate hardware test items.
 * Do not power the inverter until LKS560 inputs, startup states,
 * and MCPWM dead-time are verified separately.
 */
#include "lks32mc07x_lib.h"
#include "board_init.h"
#include "board_pwm_pins.h"

#define BOARD_HEARTBEAT_IO  (GPIO_Pin_6)

void board_pwm_inputs_off(void)
{
    GPIO_InitTypeDef gpio;
    unsigned int pin;

    SYS_ModuleClockCmd(SYS_Module_GPIO, ENABLE);

    /* Preload OFF before changing mux/direction. Callers must clear MOE
     * first if MCPWM is active; otherwise muxing one pad at a time could
     * expose an intermediate command. LKS560 OFF is HIN=0, LIN=1.
     */
    GPIO_ResetBits(GPIO1, BOARD_PWM_HIGH_SIDE_PINS);
    GPIO_SetBits(GPIO1, BOARD_PWM_LOW_SIDE_PINS);
    for (pin = BOARD_PWM_FIRST_PIN; pin <= BOARD_PWM_LAST_PIN; ++pin) {
        GPIO_PinAFConfig(GPIO1, pin, AF0_GPIO);
    }
    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = BOARD_PWM_PIN_MASK;
    gpio.GPIO_Mode = GPIO_Mode_OUT;
    gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIO1, &gpio);
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

    board_pwm_inputs_off(); // 初始化PWM通道信号输出

    /* Vendor SDK V3.16, internal oscillator: 48 MHz for A0 testing. */
    SYS_MclkChoice(SYS_MCLK_48M_RC); // 选择系统时钟为48MHz内部RC振荡器, PLL来源于内部8Mhz

    /* Bench-only policy: no unexpected watchdog reset in A0. */
    IWDG_DISABLE(); // 防止仿真时看门狗复位, 实际使用需要开启

    board_init_heartbeat(); // 初始化心跳灯, 用于指示MCU工作状态
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
