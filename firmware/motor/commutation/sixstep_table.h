#ifndef SIXSTEP_TABLE_H
#define SIXSTEP_TABLE_H
#include <stdint.h>
typedef struct { uint8_t pwm_phase, sink_phase, floating_phase; } SixstepState;
/* U=0, V=1, W=2; invalid requests leave a disabled sentinel state. */
int sixstep_get(uint32_t sector, SixstepState *state);
int sixstep_next(uint32_t sector, uint32_t reverse, uint8_t *next);
int sixstep_duty_valid(uint32_t permille);
#endif
