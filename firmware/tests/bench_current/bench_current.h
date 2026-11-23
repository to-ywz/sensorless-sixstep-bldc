#ifndef BENCH_CURRENT_H
#define BENCH_CURRENT_H

#include "../../motor/current/current_sample.h"

/* Pause at entry to bench_current_poll before reading the mailbox.
 * For APPLY, write requested_sector first, then command. Resume to consume
 * the command once. STOP holds until a subsequent APPLY.
 */
enum { BENCH_CURRENT_APPLY = 1, BENCH_CURRENT_STOP = 2 };
extern volatile uint32_t g_m3_command, g_m3_requested_sector;
extern volatile uint32_t g_m3_running, g_m3_frame_count, g_m3_timing_faults;
/* Historical values are retained across STOP/restart. Treat them as current
 * only when frame_current is true; inspect the frame while the CPU is halted.
 */
extern volatile CurrentSampleFrame g_m3_frame;
extern volatile bool g_m3_frame_current;

/* Last PWM-driven finish decision. Match pwm_cycle to the ADC snapshot. */
typedef struct {
    uint32_t finish_count;
    uint32_t pwm_cycle;
    uint32_t elapsed_ticks;
    int16_t counter_tick;
    bool boundary;
    bool compare;
} BenchCurrentPwmDebug;

extern volatile BenchCurrentPwmDebug g_m3_pwm_debug;

void bench_current_init(const CurrentCalibration *board_calibration);
void bench_current_poll(void);

#endif
