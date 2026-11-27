#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H
#include "protocol_justfloat.h"

typedef void (*DiagnosticSource)(DiagnosticSnapshot *snapshot);
void diagnostic_init(DiagnosticSource read_source, uint32_t ticks_per_second);
void diagnostic_poll(uint32_t ticks);
void diagnostic_log(unsigned level, const char *message);
#endif
