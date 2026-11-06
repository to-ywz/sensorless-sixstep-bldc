/**
 * @file test_mcpwm_single.h
 * @brief V0.1-A1 single-pin MCPWM laboratory experiment.
 *
 * Never use this experiment with energized power-stage rails or a motor.
 */
#ifndef TEST_MCPWM_SINGLE_H
#define TEST_MCPWM_SINGLE_H

/** @brief Initialize MCPWM and START1/STOP1 inputs; PWM remains disabled. */
void bench_mcpwm_single_init(void);

/** @brief Enable CH0P on MCU pin P1.4 for oscilloscope observation. */
void bench_mcpwm_single_enable(void);

/** @brief Disable MCPWM and restore P1.4 as a low-level GPIO. */
void bench_mcpwm_single_disable(void);

/**
 * @brief Poll the physical START1/STOP1 buttons.
 *
 * START1 is debounced before enabling PWM. STOP1 is active-low and
 * requests immediate software disable; it always has priority.
 */
void bench_mcpwm_single_poll(void);

#endif /* TEST_MCPWM_SINGLE_H */
