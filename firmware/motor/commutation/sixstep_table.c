#include "sixstep_table.h"
static const SixstepState states[6] = {
    {0u,1u,2u}, {0u,2u,1u}, {1u,2u,0u},
    {1u,0u,2u}, {2u,0u,1u}, {2u,1u,0u}
};
int sixstep_get(uint32_t sector, SixstepState *state)
{
    if (state == 0) return 0;
    state->pwm_phase = state->sink_phase = state->floating_phase = 255u;
    if (sector >= 6u) return 0;
    *state = states[sector];
    return 1;
}
int sixstep_next(uint32_t sector, uint32_t reverse, uint8_t *next)
{
    if (next == 0) return 0;
    *next = 255u;
    if (sector >= 6u || reverse > 1u) return 0;
    *next = (uint8_t)(reverse ? (sector == 0u ? 5u : sector-1u) :
                               (sector == 5u ? 0u : sector+1u));
    return 1;
}
int sixstep_duty_valid(uint32_t permille)
{
    return permille >= 100u && permille <= 500u;
}
