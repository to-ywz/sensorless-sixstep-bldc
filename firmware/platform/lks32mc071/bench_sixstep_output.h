#ifndef BENCH_SIXSTEP_OUTPUT_H
#define BENCH_SIXSTEP_OUTPUT_H
#include <stdint.h>

enum { BENCH_SIXSTEP_HALF_PERIOD_TICKS = 2400 };

typedef void (*BenchPwmBeforeRestart)(void);
/* Shared isolated-bench backend; not qualified for energized-bus operation. */
void bench_sixstep_output_init(void);
void bench_sixstep_output_stop(void);
int bench_sixstep_output_apply(uint32_t sector, uint32_t duty_permille);
/* Optional preparation while CNT0 is stopped, immediately before restart. */
int bench_sixstep_output_apply_synced(uint32_t sector, uint32_t duty_permille,
                                    BenchPwmBeforeRestart prepare);
/* Existing isolated-bench STOP input. */
void bench_sixstep_stop_input_init(void);
int bench_sixstep_stop_requested(void);
/* A5: one selected gate request only; no second conducting phase.
 * high_pwm=0: selected low-side ON; high_pwm=1: selected high-side PWM.
 * Fixed 250 permille carrier for high-side observation. Invalid => all OFF.
 */
int bench_gate_output_apply(uint32_t phase, uint32_t high_pwm);
uint32_t bench_sixstep_clock(void);
uint32_t bench_sixstep_ticks_per_ms(void);
#endif
