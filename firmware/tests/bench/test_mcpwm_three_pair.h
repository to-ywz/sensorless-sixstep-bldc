/**
 * @file test_mcpwm_three_pair.h
 * @brief V0.1-A3 three-pair complementary PWM logic-only bench test.
 *
 * Power bus must remain disconnected and no motor may be connected.
 */
#ifndef TEST_MCPWM_THREE_PAIR_H
#define TEST_MCPWM_THREE_PAIR_H

/** @brief Configure CH0/1/2 P/N complementary PWM and START1/STOP1 inputs. */
void bench_mcpwm_three_pair_init(void);

/** @brief Poll START1/STOP1; STOP has priority. */
void bench_mcpwm_three_pair_poll(void);

/** @brief Enable CH0/1/2 P/N on P1.4..P1.9 for scope observation. */
void bench_mcpwm_three_pair_enable(void);

/** @brief Disable MCPWM and restore P1.4..P1.9 as GPIO HIN-low/LIN-high. */
void bench_mcpwm_three_pair_disable(void);

#endif /* TEST_MCPWM_THREE_PAIR_H */
