#include "current_sample.h"
#include "../commutation/sixstep_table.h"

CurrentTrigger current_trigger(uint32_t cycle, uint8_t sector, bool in_window,
                               bool static_sample, const CurrentCalibration *calibration)
{
    CurrentTrigger trigger = {0};
    SixstepState state;

    trigger.pwm_cycle = cycle;
    trigger.sector = sector;
    trigger.in_window = in_window;
    trigger.calibration = *calibration;
    if (sixstep_get(sector, &state)) {
        trigger.phase_mask = static_sample ? CURRENT_ALL_PHASES : (1u << state.sink_phase);
    }
    return trigger;
}

int16_t current_decode_adc(uint16_t word, bool right_aligned)
{
    int32_t code = right_aligned ? (word & 0xfffu) : (word >> 4);
    if (code >= 2048) {
        code -= 4096;
    }
    return (int16_t)code;
}

CurrentSampleFrame current_sample(const CurrentTrigger *trigger,
                                  const uint16_t words[CURRENT_PHASES],
                                  uint8_t received_mask, bool right_aligned,
                                  uint32_t now_cycle, uint32_t max_age)
{
    CurrentSampleFrame frame = {0};
    uint8_t phase;

    frame.pwm_cycle = trigger->pwm_cycle;
    frame.sector = trigger->sector;
    frame.calibration_version = trigger->calibration.version;
    frame.raw_mask = received_mask & CURRENT_ALL_PHASES;
    for (phase = 0; phase < CURRENT_PHASES; ++phase) {
        uint8_t bit = 1u << phase;
        CurrentPhaseSample *sample = &frame.phase[phase];
        const CurrentPhaseCalibration *cal = &trigger->calibration.phase[phase];

        if (frame.raw_mask & bit) {
            sample->raw = current_decode_adc(words[phase], right_aligned);
        }
        if (trigger->phase_mask == 0) {
            sample->status = CURRENT_BAD_SECTOR;
        } else if (!(trigger->phase_mask & bit)) {
            sample->status = CURRENT_NOT_SELECTED;
        } else if (!trigger->in_window) {
            sample->status = CURRENT_OUTSIDE_WINDOW;
        } else if (!(frame.raw_mask & bit)) {
            sample->status = CURRENT_MISSING;
        } else if (now_cycle - trigger->pwm_cycle > max_age) {
            sample->status = CURRENT_EXPIRED;
        } else if (sample->raw == -2048 || sample->raw == 2047) {
            sample->status = CURRENT_SATURATED;
        } else if (trigger->calibration.version == 0 || cal->microamps_per_code == 0 ||
                   cal->zero_code < -2048 || cal->zero_code > 2047) {
            sample->status = CURRENT_UNCALIBRATED;
        } else {
            sample->microamps = ((int64_t)sample->raw - cal->zero_code) * cal->microamps_per_code;
            frame.valid_mask |= bit;
        }
    }
    return frame;
}
