#include "protocol_justfloat.h"
#include <float.h>
#include <string.h>

typedef char JustFloatRequiresBinary32[
    sizeof(float) == 4 && FLT_RADIX == 2 && FLT_MANT_DIG == 24 && FLT_MAX_EXP == 128 ? 1 : -1];

void justfloat_store(uint8_t bytes[4], float value)
{
    uint32_t bits;
    unsigned i;
    memcpy(&bits, &value, sizeof bits);
    /* Map infinities and NaNs to the same zero used for unavailable values. */
    if ((bits & 0x7f800000u) == 0x7f800000u) {
        bits = 0;
    }
    for (i = 0; i < 4; ++i) {
        bytes[i] = (uint8_t)(bits >> (8u * i));
    }
}

void justfloat_channel(uint8_t bytes[4], const DiagnosticSnapshot *snapshot,
                       unsigned channel, uint32_t sequence)
{
    const CurrentSampleFrame *frame = &snapshot->frame;
    unsigned valid = snapshot->frame_current && frame->calibration_version ?
                     frame->valid_mask : 0;
    float value = 0.0f;
    if (channel < 3) {
        if (frame->raw_mask & (1u << channel)) {
            value = (float)frame->phase[channel].raw;
        }
    } else if (channel < 6) {
        unsigned phase = channel - 3u;
        if (valid & (1u << phase)) {
            value = (float)frame->phase[phase].microamps * 1.0e-6f;
        }
    } else {
        switch (channel) {
        case 6: value = (float)frame->sector; break;
        case 7: value = (float)(frame->pwm_cycle & 0xffffffu); break;
        case 8: value = (float)snapshot->running; break;
        case 9: value = (float)snapshot->frame_current; break;
        case 10: value = (float)frame->raw_mask; break;
        case 11: value = (float)valid; break;
        case 12: case 13: case 14:
            value = (float)(snapshot->sample_count ?
                            frame->phase[channel - 12u].status : CURRENT_MISSING);
            break;
        case 15: value = (float)(sequence & 0xffffffu); break;
        default: break;
        }
    }
    justfloat_store(bytes, value);
}
