#ifndef PROTOCOL_TEXT_H
#define PROTOCOL_TEXT_H
#include <stddef.h>
#include <stdint.h>
typedef enum {
    DIAG_UNKNOWN, DIAG_HELP, DIAG_VERSION, DIAG_STATUS, DIAG_SAMPLE, DIAG_STATS,
    DIAG_STREAM, DIAG_RATE, DIAG_LOG, DIAG_MODE_TEXT, DIAG_MODE_JUSTFLOAT
} DiagnosticCommand;
DiagnosticCommand protocol_text_parse(const char *line, unsigned *value);
enum { PROTOCOL_TEXT_FIELD_BYTES = 80 };
size_t protocol_text_fields(char text[PROTOCOL_TEXT_FIELD_BYTES], const char *label, uint32_t a, uint32_t b, uint32_t c, uint32_t d);
#endif
