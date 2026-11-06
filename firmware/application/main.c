/**
 * @file main.c
 * @brief V0.1-A0 safe board bring-up entry.
 *
 * The power stage must be unpowered and the motor disconnected in A0.
 * This program intentionally does not configure or enable MCPWM.
 */
#include <stdint.h>
#include "lks32mc07x_lib.h"
#include "board_init.h"

/**
 * @brief Wait for an approximate delay; this is not a calibrated timer.
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

    for (;;) {
        board_heartbeat_toggle();
        bench_delay(600000u);
    }
}
