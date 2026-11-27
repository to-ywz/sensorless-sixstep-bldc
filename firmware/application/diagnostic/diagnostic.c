#include "diagnostic.h"
#ifdef BENCH_DEBUG_UART
#include "protocol_text.h"
#include "../../platform/lks32mc071/lks_uart.h"
#include <string.h>

enum { TEXT_CAPACITY = 32, NO_SWITCH = 0, TO_TEXT, TO_JUSTFLOAT };
static DiagnosticSource source;
static DiagnosticSnapshot snapshot;
static uint8_t frame[JUSTFLOAT_SIZE];
static char line[TEXT_CAPACITY];
static unsigned used, stage, mode_switch, log_level;
static uint32_t clock_hz, interval, last, sequence, rejected;
static bool text, streaming, discard, receive_turn, previous_running;

static void reply(const char *message)
{
    size_t length = 0;
    if (!text || mode_switch) {
        return;
    }
    while (length < 80u && message[length]) {
        ++length;
    }
    if (length && length < 80u) {
        (void)lks_uart_send(message, length);
    }
}

void diagnostic_log(unsigned level, const char *message)
{
    if (level && level <= log_level) {
        reply(message);
    }
}

void diagnostic_init(DiagnosticSource read_source, uint32_t ticks_per_second)
{
    source = read_source;
    clock_hz = ticks_per_second;
    interval = clock_hz / 20u;
    streaming = true;
    text = discard = receive_turn = previous_running = false;
    used = stage = mode_switch = 0;
    last = sequence = rejected = 0;
    log_level = 2;
}

static void reply_fields(const char *label, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    char bytes[PROTOCOL_TEXT_FIELD_BYTES];
    size_t length = protocol_text_fields(bytes, label, a, b, c, d);
    (void)lks_uart_send(bytes, length);
}

static void command(void)
{
    DiagnosticSnapshot state;
    LksUartStats stats;
    unsigned rate;
    switch (protocol_text_parse(line, &rate)) {
    case DIAG_HELP:
        reply("HELP VERSION STATUS SAMPLE STATS STREAM RATE LOG MODE\r\n");
        break;
    case DIAG_VERSION:
        reply("V0.2 DEBUG-01 JustFloat-M3/1\r\n");
        break;
    case DIAG_STATUS:
        if (text) {
            source(&state);
            reply_fields("STATUS running,current,count,faults ",
                                 state.running, state.frame_current,
                                 state.sample_count, state.timing_faults);
        }
        break;
    case DIAG_SAMPLE:
        if (text) {
            source(&state);
            reply_fields("SAMPLE raw_mask,U,V,W ", state.frame.raw_mask,
                                 (uint32_t)state.frame.phase[0].raw,
                                 (uint32_t)state.frame.phase[1].raw,
                                 (uint32_t)state.frame.phase[2].raw);
        }
        break;
    case DIAG_STATS:
        if (text) {
            stats = lks_uart_stats();
            reply_fields("STATS drop,overflow,error,rejected ",
                                 stats.tx_dropped, stats.rx_overflow,
                                 stats.rx_errors, rejected);
        }
        break;
    case DIAG_STREAM:
        streaming = rate != 0;
        stage = 0;
        reply("OK\r\n");
        break;
    case DIAG_RATE:
        interval = clock_hz / rate;
        reply("OK\r\n");
        break;
    case DIAG_LOG:
        log_level = rate;
        reply("OK\r\n");
        break;
    case DIAG_MODE_TEXT:
        if (!text) {
            mode_switch = TO_TEXT;
            stage = 0;
        }
        break;
    case DIAG_MODE_JUSTFLOAT:
        if (text) {
            mode_switch = TO_JUSTFLOAT;
        }
        break;
    default:
        ++rejected;
        reply("ERR command\r\n");
        break;
    }
}

static void receive_one(void)
{
    uint8_t byte;
    bool gap;
    bool available = lks_uart_receive(&byte, &gap);
    if (gap) {
        discard = true;
    }
    if (!available) {
        return;
    }
    if (byte == '\r' || byte == '\n') {
        if (!discard && used) {
            line[used] = '\0';
            command();
        } else if (discard) {
            ++rejected;
            reply("ERR line\r\n");
        }
        used = 0;
        discard = false;
    } else if (!discard) {
        if (used == sizeof line - 1 || byte < 32 || byte > 126) {
            discard = true;
        } else {
            line[used++] = (char)byte;
        }
    }
}

void diagnostic_poll(uint32_t ticks)
{
    DiagnosticSnapshot latest;
    if (mode_switch) {
        if (lks_uart_idle()) {
            text = mode_switch == TO_TEXT;
            mode_switch = NO_SWITCH;
            reply("OK TEXT\r\n");
        }
        return;
    }

    /* One byte per visit. Alternate with telemetry even under continuous RX. */
    receive_turn = !receive_turn;
    if (receive_turn) {
        receive_one();
        return;
    }
    if (text) {
        if ((uint32_t)(ticks - last) >= interval) {
            last = ticks;
            source(&latest);
            if (latest.running != previous_running) {
                previous_running = latest.running;
                reply(latest.running ? "EVENT PWM RUNNING\r\n" : "EVENT PWM STOPPED\r\n");
            }
        }
        return;
    }
    if (!streaming) {
        return;
    }
    if (!stage) {
        if ((uint32_t)(ticks - last) >= interval) {
            last = ticks;
            ++sequence;
            source(&snapshot);
            stage = 1;
        }
    } else if (stage <= JUSTFLOAT_CHANNELS) {
        justfloat_channel(&frame[(stage - 1u) * 4u], &snapshot, stage - 1u, sequence);
        ++stage;
    } else {
        source(&latest);
        /* Abort a staged current snapshot invalidated by stop or restart. */
        if (latest.running == snapshot.running &&
            latest.frame_current == snapshot.frame_current) {
            static const uint8_t tail[] = {0, 0, 0x80, 0x7f};
            memcpy(&frame[JUSTFLOAT_SIZE - sizeof tail], tail, sizeof tail);
            (void)lks_uart_send(frame, sizeof frame);
        }
        stage = 0;
    }
}
#endif
