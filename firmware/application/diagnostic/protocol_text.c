#include "protocol_text.h"
#include <string.h>

DiagnosticCommand protocol_text_parse(const char *line, unsigned *value)
{
    static const char *const names[] = {
        "HELP", "VERSION", "STATUS", "SAMPLE", "STATS", "STREAM", "RATE", "LOG",
        "MODE TEXT", "MODE JUSTFLOAT"
    };
    unsigned i;
    for (i = 0; i < sizeof names / sizeof names[0]; ++i) {
        DiagnosticCommand kind = (DiagnosticCommand)(i + 1);
        size_t length = strlen(names[i]);
        if (strncmp(line, names[i], length) != 0) {
            continue;
        }
        if (kind == DIAG_STREAM || kind == DIAG_RATE || kind == DIAG_LOG) {
            const char *p = line + length;
            unsigned number = 0, digits = 0;
            if (*p++ != ' ') {
                return DIAG_UNKNOWN;
            }
            while (*p >= '0' && *p <= '9' && digits < 3) {
                number = 10u * number + (unsigned)(*p++ - '0');
                ++digits;
            }
            if (*p || !digits ||
                (kind == DIAG_STREAM && number > 1u) ||
                (kind == DIAG_RATE && (number == 0u || number > 100u)) ||
                (kind == DIAG_LOG && number > 3u)) {
                return DIAG_UNKNOWN;
            }
            *value = number;
            return kind;
        }
        if (!line[length]) {
            return kind;
        }
    }
    return DIAG_UNKNOWN;
}

size_t protocol_text_fields(char text[PROTOCOL_TEXT_FIELD_BYTES], const char *label, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    static const char hex[] = "0123456789ABCDEF";
    const uint32_t fields[] = {a, b, c, d};
    size_t pos = strlen(label);
    unsigned i, digit;
    if (pos > PROTOCOL_TEXT_FIELD_BYTES - 38u) {
        return 0;
    }
    memcpy(text, label, pos);
    for (i = 0; i < 4; ++i) {
        for (digit = 0; digit < 8; ++digit) {
            text[pos++] = hex[(fields[i] >> (28u - 4u * digit)) & 15u];
        }
        text[pos++] = i == 3 ? '\r' : ',';
    }
    text[pos++] = '\n';
    return pos;
}
