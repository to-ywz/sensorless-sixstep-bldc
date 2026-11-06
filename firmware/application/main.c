/**
 * @file main.c
 * @brief V0.1 staged MCU071 board bring-up entry point.
 *
 * Default: A0 GPIO-safe heartbeat.
 * Opt-in define BENCH_TEST_MCPWM_SINGLE: A1 logic-only PWM at P1.4.
 *
 * Never energize the inverter power bus or connect the motor in A0/A1.
 */
#include <stdint.h>
#include "lks32mc07x_lib.h"
#include "board_init.h"

#if defined(BENCH_TEST_MCPWM_SINGLE)
#include "test_mcpwm_single.h"
#endif

/**
 * @brief Approximate, noncalibrated bench delay.
 */
static void bench_delay(volatile uint32_t iterations)
{
    while (iterations-- != 0u) {
        __NOP();
    }
}

int main(void)
{
    board_init_safe();

#if defined(BENCH_TEST_MCPWM_SINGLE)
    /*
     * A1 mode is only for MCU logic-pin observation with the
     * external inverter unpowered and FN550-90K disconnected.
     */
    bench_mcpwm_single_init();

    for (;;) {
        bench_mcpwm_single_enable();
        bench_delay(4000000u);
        bench_mcpwm_single_disable();
        bench_delay(4000000u);
    }
#else
    /* Validated A0 program remains the default. */
    for (;;) {
        board_heartbeat_toggle();
        bench_delay(600000u);
    }
#endif
}
