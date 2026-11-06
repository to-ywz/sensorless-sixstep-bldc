/**
 * @file test_mcpwm_single.h
 * @brief V0.1-A1 single-pin MCPWM laboratory experiment.
 *
 * Never use this experiment with energized power-stage rails or a motor.
 */
#ifndef TEST_MCPWM_SINGLE_H
#define TEST_MCPWM_SINGLE_H

/** @brief Initialize the MCPWM but leave the observed pin disabled. */
void bench_mcpwm_single_init(void);

/** @brief Enable CH0P on MCU pin P1.4 for oscilloscope observation. */
void bench_mcpwm_single_enable(void);

/** @brief Disable MCPWM and restore P1.4 as a low-level GPIO. */
void bench_mcpwm_single_disable(void);

#endif /* TEST_MCPWM_SINGLE_H */
