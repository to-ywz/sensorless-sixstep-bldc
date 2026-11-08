/**
 * @file main.c
 * @brief V0.1 staged MCU071 board bring-up entry point.
 *
 * Default: A0 GPIO-safe heartbeat.
 * Opt-in define BENCH_TEST_MCPWM_SINGLE: A1 single-output PWM at P1.4.
 * Opt-in define BENCH_TEST_MCPWM_COMPLEMENTARY: A2 CH0P/CH0N pair.
 *
 * Opt-in define BENCH_TEST_MCPWM_THREE_PAIR: A3 CH0/1/2 P/N.
 *
 * Never energize the inverter power bus or connect the motor in A0/A1/A2/A3.
 */
#include <stdint.h>
#include "lks32mc07x_lib.h"
#include "board_init.h"

#if (defined(BENCH_TEST_MCPWM_SINGLE) + defined(BENCH_TEST_MCPWM_COMPLEMENTARY) + \
     defined(BENCH_TEST_MCPWM_THREE_PAIR)) > 1
#error "Select only one MCPWM bench test"
#endif

#if defined(BENCH_TEST_MCPWM_THREE_PAIR)
#include "test_mcpwm_three_pair.h"
#elif defined(BENCH_TEST_MCPWM_COMPLEMENTARY)
#include "test_mcpwm_complementary.h"
#elif defined(BENCH_TEST_MCPWM_SINGLE)
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

#if defined(BENCH_TEST_MCPWM_THREE_PAIR)
    /* A3: logic pins only; disconnect inverter DC bus and motor. */
    bench_mcpwm_three_pair_init();
    for (;;) {
        bench_mcpwm_three_pair_poll();
        bench_delay(12000u);
    }
#elif defined(BENCH_TEST_MCPWM_COMPLEMENTARY)
    /*
     * A2 mode: CH0P/CH0N complementary PWM, logic-only observation.
     * Keep the inverter DC bus disconnected and motor unplugged.
     */
    bench_mcpwm_complementary_init();

    for (;;) {
        bench_mcpwm_complementary_poll();
        bench_delay(12000u);
    }
#elif defined(BENCH_TEST_MCPWM_SINGLE)
    /*
     * A1 mode is only for MCU logic-pin observation with the
     * inverter power stage unpowered and FN550-90K disconnected.
     */
    bench_mcpwm_single_init();

    for (;;) {
        bench_mcpwm_single_poll();
        /* Approximate scan interval; debounce count is verified on bench. */
        bench_delay(12000u);
    }
#else
    /* Validated A0 program remains the default. */
    for (;;) {
        board_heartbeat_toggle();
        bench_delay(600000u);
    }
#endif
}
