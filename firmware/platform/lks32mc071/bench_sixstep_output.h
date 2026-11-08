#ifndef BENCH_SIXSTEP_OUTPUT_H
#define BENCH_SIXSTEP_OUTPUT_H
#include <stdint.h>
/* Logic-only bench driver; not a qualified power-stage driver. */
void bench_sixstep_output_init(void);
void bench_sixstep_output_stop(void);
int bench_sixstep_output_apply(uint32_t sector, uint32_t duty_permille);
uint32_t bench_sixstep_clock(void);
uint32_t bench_sixstep_ticks_per_ms(void);
#endif
