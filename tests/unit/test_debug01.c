#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "lks32mc07x_lib.h"
#include "../../firmware/platform/lks32mc071/lks_uart.h"
#include "../../firmware/application/diagnostic/diagnostic.h"
#include "../../firmware/application/diagnostic/protocol_text.h"

MockUart mock_uart;
static uint32_t now;
static DiagnosticSnapshot snapshot;

static void source(DiagnosticSnapshot *out)
{
    *out = snapshot;
}

static void rx_byte(uint8_t byte, uint32_t error)
{
    uint32_t transmitting_byte = UART0->BUFF;
    UART0->BUFF = byte;
    UART0->IF = UART_IF_RcvOver | error;
    lks_uart_irq();
    UART0->BUFF = transmitting_byte;
}

static void command(const char *line)
{
    while (*line) {
        rx_byte((uint8_t)*line++, 0);
        diagnostic_poll(now);
        diagnostic_poll(now);
    }
}

static size_t drain(uint8_t bytes[256])
{
    size_t used = 0;
    while (!lks_uart_idle()) {
        assert(used < 256);
        bytes[used++] = (uint8_t)UART0->BUFF;
        UART0->IF = UART_IF_SendOver;
        lks_uart_irq();
    }
    return used;
}

int main(void)
{
    uint8_t bytes[256], payload[68], value;
    bool gap;
    unsigned i, rate;
    const uint8_t expected[] = {
        0,0,0x80,0x3f, 0,0,0,0xc0, 0,0,0,0, 0,0,0,0,
        0,0,0,0, 0,0,0,0, 0,0,0xa0,0x40, 0xff,0xff,0x7f,0x4b,
        0,0,0x80,0x3f, 0,0,0x80,0x3f, 0,0,0x40,0x40, 0,0,0,0,
        0,0,0xe0,0x40, 0,0,0xe0,0x40, 0,0,0x80,0x3f, 0,0,0x80,0x3f,
        0,0,0x80,0x7f
    };

    lks_uart_init();
#ifdef BENCH_DEBUG_UART_3M_EXPERIMENTAL
    assert(UART0->DIVL == 15 && UART0->DIVH == 0);
#else
    assert(UART0->DIVL == 160 && UART0->DIVH == 1);
#endif
    assert(lks_uart_idle() && !lks_uart_receive(&value, &gap) && !gap);
    /* Mock BUFF represents the wire byte while draining continuous TX. */
    for (i = 0; i < sizeof payload; ++i) {
        payload[i] = (uint8_t)i;
    }
    for (i = 0; i < 3; ++i) {
        assert(lks_uart_send(payload, sizeof payload));
    }
    assert(!lks_uart_send(payload, sizeof payload));
    assert(drain(bytes) == 204);
    for (i = 0; i < 3; ++i) {
        assert(memcmp(bytes + i * sizeof payload, payload, sizeof payload) == 0);
    }
    assert(lks_uart_send(payload, sizeof payload));
    assert(drain(bytes) == sizeof payload && memcmp(bytes, payload, sizeof payload) == 0);
    for (i = 0; i < 250; ++i) {
        rx_byte((uint8_t)i, 0);
        assert(lks_uart_receive(&value, &gap) && !gap && value == (uint8_t)i);
    }
    for (i = 0; i < 65; ++i) {
        rx_byte('a', 0);
    }
    assert(!lks_uart_receive(&value, &gap) && gap);
    rx_byte('b', UART_IF_StopError);
    assert(!lks_uart_receive(&value, &gap) && gap);
    assert(lks_uart_stats().tx_dropped == 1);
    assert(lks_uart_stats().rx_overflow == 2 && lks_uart_stats().rx_errors == 1);

    justfloat_store(bytes, INFINITY);
    assert(memcmp(bytes, "\x00\x00\x00\x00", 4) == 0);
    justfloat_store(bytes, -1.0f);
    assert(memcmp(bytes, "\x00\x00\x80\xbf", 4) == 0);
    snapshot.frame.pwm_cycle = UINT32_MAX;
    snapshot.frame.sector = 5;
    snapshot.frame.raw_mask = 3;
    snapshot.frame.phase[0].raw = 1;
    snapshot.frame.phase[1].raw = -2;
    snapshot.frame.phase[0].status = CURRENT_UNCALIBRATED;
    snapshot.frame.phase[1].status = CURRENT_UNCALIBRATED;
    snapshot.frame.phase[2].status = CURRENT_NOT_SELECTED;
    snapshot.running = snapshot.frame_current = true;
    snapshot.sample_count = 1;
    justfloat_channel(bytes, &snapshot, 7, 0);
    assert(memcmp(bytes, expected + 28, 4) == 0);
    justfloat_channel(bytes, &snapshot, 15, 0x1000000u);
    assert(memcmp(bytes, "\x00\x00\x00\x00", 4) == 0);
    assert(protocol_text_parse("RATE 100", &rate) == DIAG_RATE && rate == 100);
    assert(protocol_text_parse("RATE 101", &rate) == DIAG_UNKNOWN);
    assert(protocol_text_parse("RATE 0", &rate) == DIAG_UNKNOWN);
    assert(protocol_text_parse("STREAM 2", &rate) == DIAG_UNKNOWN);
    assert(protocol_text_parse("RUN", &rate) == DIAG_UNKNOWN);

    diagnostic_init(source, 1000);
    command("VERSION\n");
    assert(lks_uart_idle());
    now = 50;
    for (i = 0; i < 50; ++i) {
        diagnostic_poll(now);
    }
    assert(drain(bytes) == sizeof expected && memcmp(bytes, expected, sizeof expected) == 0);
    now += 50;
    for (i = 0; i < 50; ++i) {
        diagnostic_poll(now);
    }
    assert(!lks_uart_idle());
    command("MODE TEXT");
    assert(!lks_uart_idle());
    command("\r\n");
    assert(drain(bytes) == sizeof expected && memcmp(bytes, expected, 60) == 0);
    diagnostic_poll(now);
    assert(drain(bytes) == 9 && memcmp(bytes, "OK TEXT\r\n", 9) == 0);
    for (i = 0; i < 4; ++i) {
        diagnostic_poll(now);
    }
    command("ABCDEFGHIJKLMNOPQRSTUVWXYZ01234567\n");
    assert(drain(bytes) == 10 && memcmp(bytes, "ERR line\r\n", 10) == 0);
    command("RUN\n");
    assert(drain(bytes) == 13 && memcmp(bytes, "ERR command\r\n", 13) == 0);
    command("STREAM 0\n");
    assert(drain(bytes) == 4);
    command("MODE JUSTFLOAT\n");
    now += 500;
    for (i = 0; i < 50; ++i) {
        diagnostic_poll(now);
    }
    assert(lks_uart_idle());
    command("RATE 100\nSTREAM 1\n");
    now += 10;
    for (i = 0; i < 50; ++i) {
        diagnostic_poll(now);
    }
    assert(drain(bytes) == JUSTFLOAT_SIZE);
    snapshot.running = snapshot.frame_current = false;
    snapshot.frame.valid_mask = 3;
    snapshot.frame.calibration_version = 1;
    for (unsigned period = 0; period < 3 && lks_uart_idle(); ++period) {
        now += 10;
        for (i = 0; i < 50 && lks_uart_idle(); ++i) {
            diagnostic_poll(now);
        }
    }
    assert(drain(bytes) == JUSTFLOAT_SIZE);
    assert(memcmp(bytes + 32, "\x00\x00\x00\x00\x00\x00\x00\x00", 8) == 0);
    assert(memcmp(bytes + 44, "\x00\x00\x00\x00", 4) == 0);
    puts("DEBUG-01 transport, protocol and diagnostic mode Mock PASS");
    return 0;
}
