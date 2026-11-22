#ifndef CURRENT_SAMPLE_H
#define CURRENT_SAMPLE_H

#include <stdbool.h>
#include <stdint.h>

enum { CURRENT_PHASES = 3, CURRENT_ALL_PHASES = 7 };

typedef enum {
    CURRENT_OK,
    CURRENT_NOT_SELECTED,
    CURRENT_BAD_SECTOR,
    CURRENT_OUTSIDE_WINDOW,
    CURRENT_MISSING,
    CURRENT_EXPIRED,
    CURRENT_SATURATED,
    CURRENT_UNCALIBRATED
} CurrentSampleStatus;

typedef struct {
    int16_t zero_code;
    int32_t microamps_per_code;
} CurrentPhaseCalibration;

typedef struct {
    uint32_t version;
    CurrentPhaseCalibration phase[CURRENT_PHASES];
} CurrentCalibration;

/* Capture by value from the effective PWM state. Sector numbering follows sixstep_get. */
typedef struct {
    uint32_t pwm_cycle;
    uint8_t sector;
    uint8_t phase_mask;
    bool in_window;
    CurrentCalibration calibration;
} CurrentTrigger;

typedef struct {
    int16_t raw;
    int64_t microamps;
    CurrentSampleStatus status;
} CurrentPhaseSample;

typedef struct {
    uint32_t pwm_cycle;
    uint32_t calibration_version;
    uint8_t sector;
    uint8_t raw_mask;
    uint8_t valid_mask;
    CurrentPhaseSample phase[CURRENT_PHASES];
} CurrentSampleFrame;

/* Only the static/Mock path selects all phases. The PWM path selects the sink shunt. */
CurrentTrigger current_trigger(uint32_t cycle, uint8_t sector, bool in_window,
                               bool static_sample, const CurrentCalibration *calibration);
int16_t current_decode_adc(uint16_t word, bool right_aligned);
/* Monotonic modulo-32-bit cycle counter. max_age is at most UINT32_MAX/2. */
CurrentSampleFrame current_sample(const CurrentTrigger *trigger,
                                  const uint16_t words[CURRENT_PHASES],
                                  uint8_t received_mask, bool right_aligned,
                                  uint32_t now_cycle, uint32_t max_age);

#endif
