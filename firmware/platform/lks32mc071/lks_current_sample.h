#ifndef LKS_CURRENT_SAMPLE_H
#define LKS_CURRENT_SAMPLE_H

#include "../../motor/current/current_sample.h"

typedef struct {
    CurrentTrigger trigger;
    bool pending;
    bool needs_idle_init;
} LksCurrentSampler;

typedef struct {
    int16_t tick;
    bool boundary;
    bool compare;
} LksCurrentPwmEvent;

/* Last ADC finish attempt, retained for Watch after STOP. */
typedef struct {
    uint32_t finish_count;
    uint32_t pwm_cycle;
    uint32_t adc_flags;
    uint8_t sector;
    bool attributed;
    bool adc_idle;
} LksCurrentDebug;

extern volatile LksCurrentDebug g_m3_adc_debug;

LksCurrentPwmEvent lks_current_pwm_event(void);
void lks_current_clear_pwm_events(void);
void lks_current_cancel(LksCurrentSampler *sampler);
bool lks_current_recover(LksCurrentSampler *sampler);

/* Exclusive ADC0 and TMR0 ownership. Call with ADC idle, after MCPWM clock setup.
 * The board owner configures OPA and supplies the verified compare/window policy.
 * Serialize Init, Arm and Finish in one PWM timing owner.
 */
void lks_current_init(LksCurrentSampler *sampler, int16_t compare_tick);
/* Arm for a staged restart or an effective period, strictly before TADC0. */
bool lks_current_arm(LksCurrentSampler *sampler, uint32_t cycle, uint8_t sector,
                     bool in_window, const CurrentCalibration *calibration);
/* Call after the conversion deadline. Set before_next_compare only if no
 * subsequent TADC0 could have occurred. Failure to finish safely requires idle Init.
 */
bool lks_current_finish(LksCurrentSampler *sampler, uint32_t now_cycle,
                        bool before_next_compare, CurrentSampleFrame *frame);

#endif
