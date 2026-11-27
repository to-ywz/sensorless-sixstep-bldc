#ifndef PROTOCOL_JUSTFLOAT_H
#define PROTOCOL_JUSTFLOAT_H

#include "../../motor/current/current_sample.h"

enum { JUSTFLOAT_SIZE = 68, JUSTFLOAT_CHANNELS = 16 };
typedef struct {
    CurrentSampleFrame frame;
    uint32_t sample_count;
    uint32_t timing_faults;
    uint32_t frame_epoch; /* Internal invalidation generation, omitted from wire format. */
    bool running;
    bool frame_current;
} DiagnosticSnapshot;

void justfloat_channel(uint8_t bytes[4], const DiagnosticSnapshot *snapshot,
                       unsigned channel, uint32_t sequence);
void justfloat_store(uint8_t bytes[4], float value);
#endif
