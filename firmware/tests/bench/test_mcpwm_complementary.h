/**
 * @file test_mcpwm_complementary.h
 * @brief V0.1-A2 single complementary PWM logic-only bench test.
 *
 * Power bus must remain disconnected and no motor may be connected.
 */
#ifndef TEST_MCPWM_COMPLEMENTARY_H
#define TEST_MCPWM_COMPLEMENTARY_H

/** @brief Configure CH0P/CH0N complementary PWM and START1/STOP1 inputs. */
void bench_mcpwm_complementary_init(void);

/** @brief Poll START1/STOP1; STOP has priority. */
void bench_mcpwm_complementary_poll(void);

/** @brief Enable CH0P/CH0N on P1.4/P1.5 for scope observation. */
void bench_mcpwm_complementary_enable(void);

/** @brief Disable MCPWM and restore P1.4/P1.5 as GPIO-low. */
void bench_mcpwm_complementary_disable(void);

#endif /* TEST_MCPWM_COMPLEMENTARY_H */
