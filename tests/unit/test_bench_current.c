#include <assert.h>
#include <stdio.h>
#include "../../firmware/tests/bench_current/bench_current.h"
#include "../../firmware/platform/lks32mc071/bench_sixstep_output.h"
#include "../../firmware/platform/lks32mc071/lks_current_sample.h"

#ifdef BENCH_DEBUG_UART
#include "lks32mc07x_lib.h"
#include "../../firmware/platform/lks32mc071/lks_uart.h"
#include "../../firmware/application/diagnostic/diagnostic.h"

MockUart mock_uart;
#endif

static uint32_t clock_ticks, restart_prepare_ticks;
static uint32_t close_delay_ticks, finish_entry_ticks, publish_delay_ticks;
static unsigned arms;
static bool stopped, counter_running, startup_boundary, adc_ready;
static bool brake, inject_stop_on_finish, check_stop_order;
static LksCurrentPwmEvent event;

uint32_t bench_sixstep_clock(void) { return clock_ticks; }
void bench_sixstep_stop_input_init(void) {}
int bench_sixstep_stop_requested(void) { return brake; }
void bench_sixstep_output_stop(void) { stopped = true; }
void bench_sixstep_output_init(void) { bench_sixstep_output_stop(); }

int bench_sixstep_output_apply_synced(uint32_t sector, uint32_t duty,
                                    BenchPwmBeforeRestart prepare)
{
    assert(sector < 6 && duty == 250);
    counter_running = false;
    clock_ticks += restart_prepare_ticks;
    prepare();
    if (brake) {
        return 0;
    }
    event.tick = -BENCH_SIXSTEP_HALF_PERIOD_TICKS;
    event.boundary = startup_boundary;
    counter_running = true;
    stopped = false;
    return 1;
}

bool lks_current_sample_stopped(LksCurrentSampler *sampler, uint8_t sector,
                                const CurrentCalibration *calibration,
                                CurrentSampleFrame *frame)
{
    const uint16_t words[CURRENT_PHASES] = {0x0080, 0, 0};
    CurrentTrigger trigger = current_trigger(0, sector, true, false, calibration);
    assert(stopped && !sampler->pending);
    *frame = current_sample(&trigger, words, trigger.phase_mask, false, 0, 0);
    return true;
}

void lks_current_init(LksCurrentSampler *sampler, int16_t compare_tick)
{
    assert(compare_tick == 0);
    *sampler = (LksCurrentSampler){0};
}
LksCurrentPwmEvent lks_current_pwm_event(void) { return event; }
static void close_trigger(LksCurrentSampler *sampler)
{
    clock_ticks += close_delay_ticks;
    sampler->trigger_closed_at = clock_ticks;
    sampler->boundary_at_close = event.boundary;
    sampler->trigger_closed = true;
}

bool lks_current_close_if_ready(LksCurrentSampler *sampler)
{
    if (!adc_ready) {
        return false;
    }
    close_trigger(sampler);
    return true;
}
void lks_current_clear_pwm_events(void) { event.boundary = event.compare = false; }

void lks_current_cancel(LksCurrentSampler *sampler)
{
    if (check_stop_order) {
        assert(stopped && sampler->pending);
        check_stop_order = false;
    }
    sampler->pending = false;
    sampler->needs_idle_init = true;
    adc_ready = false;
}

bool lks_current_recover(LksCurrentSampler *sampler)
{
    assert(!sampler->pending);
    sampler->needs_idle_init = false;
    return true;
}

bool lks_current_arm(LksCurrentSampler *sampler, uint32_t cycle, uint8_t sector,
                     bool in_window, const CurrentCalibration *calibration)
{
    assert(!sampler->pending && !sampler->needs_idle_init);
    assert(!counter_running || event.tick <= -512);
    assert(!event.boundary && !event.compare);
    sampler->trigger = current_trigger(cycle, sector, in_window, false, calibration);
    sampler->pending = true;
    sampler->trigger_closed = false;
    ++arms;
    return true;
}

bool lks_current_finish(LksCurrentSampler *sampler, uint32_t cycle,
                        bool attributed, CurrentSampleFrame *frame)
{
    const uint16_t words[CURRENT_PHASES] = {0x0800, 0x0800, 0x0800};
    uint8_t mask = adc_ready && attributed ? sampler->trigger.phase_mask : 0;
    assert(sampler->pending);
    clock_ticks += finish_entry_ticks;
    assert(!attributed || sampler->trigger_closed);
    if (!sampler->trigger_closed) {
        close_trigger(sampler);
    }
    *frame = current_sample(&sampler->trigger, words, mask, false, cycle, 1);
    sampler->pending = false;
    sampler->needs_idle_init = !mask;
    adc_ready = false;
    clock_ticks += publish_delay_ticks;
    if (inject_stop_on_finish) {
        brake = true;
    }
    return true;
}

static void apply(uint8_t sector)
{
    g_m3_requested_sector = sector;
    g_m3_command = BENCH_CURRENT_APPLY;
    bench_current_poll();
}

static void tick(uint32_t delta, int16_t count, bool boundary, bool compare)
{
    clock_ticks += delta;
    event = (LksCurrentPwmEvent){count, boundary, compare};
    bench_current_poll();
#ifdef BENCH_DEBUG_UART
    if (bench_current_diagnostic_slot()) {
        diagnostic_poll(bench_sixstep_clock());
    }
#endif
}

#ifdef BENCH_DEBUG_UART
static uint32_t snapshot_epoch(void)
{
    DiagnosticSnapshot snapshot;
    bench_current_snapshot(&snapshot);
    return snapshot.frame_epoch;
}
#endif

int main(void)
{
#ifdef BENCH_DEBUG_UART
    uint32_t epoch;
#endif
    const CurrentCalibration cal = {7, {{0, 1}, {0, 1}, {0, 1}}};
    clock_ticks = UINT32_MAX - 100;
    bench_current_init(&cal);
#ifdef BENCH_DEBUG_UART
    {
        /* Hold TX completion throughout the real bench state-machine regression. */
        const uint8_t congestion[255] = {0};
        lks_uart_init();
        assert(lks_uart_send(congestion, sizeof congestion));
        assert(!lks_uart_send(congestion, 2));
        diagnostic_init(bench_current_snapshot, 1000);
    }
#endif
    assert(stopped && g_m3_running == 0 && g_m3_frame_count == 0);
    startup_boundary = false;
    apply(0);
    assert(!stopped && g_m3_running == 1 && arms == 1);
    tick(30, -2370, false, false);
    assert(arms == 1 && g_m3_frame_count == 0);
    tick(50, -2320, true, false);
    assert(arms == 1 && g_m3_frame_count == 0);
    puts("PASS: delayed restart boundary neither advances nor re-arms first PWM period");
    adc_ready = true;
    tick(2480, 160, false, true);
    assert(g_m3_frame.pwm_cycle == 1 && g_m3_frame.sector == 0);
    assert(g_m3_frame.valid_mask == 2 && g_m3_frame.calibration_version == 7);
#ifdef BENCH_DEBUG_UART
    {
        DiagnosticSnapshot observed;
        assert(bench_current_diagnostic_slot());
        bench_current_snapshot(&observed);
        assert(observed.running && observed.frame_current && observed.frame.valid_mask == 2);
        assert(observed.frame.calibration_version == 7 && observed.frame.pwm_cycle == 1);
    }
#endif
    assert(g_m3_pwm_debug.finish_count == 1 && g_m3_pwm_debug.pwm_cycle == 1);
    assert(g_m3_pwm_debug.counter_tick == 160 && g_m3_pwm_debug.elapsed_ticks == 2560);
    assert(!g_m3_pwm_debug.boundary && g_m3_pwm_debug.compare);
    tick(10, 170, false, true);
    assert(g_m3_frame_count == 1 && arms == 1);

    tick(2280, -2351, true, true);
    assert(arms == 2);
    tick(50, -2301, false, false);
    startup_boundary = false;
#ifdef BENCH_DEBUG_UART
    epoch = snapshot_epoch();
#endif
    apply(1);
#ifdef BENCH_DEBUG_UART
    assert(snapshot_epoch() == epoch + 1u);
#endif
    assert(arms == 3 && g_m3_frame.sector == 0 && g_m3_frame.pwm_cycle == 2);
    assert(g_m3_frame.valid_mask == 0 && g_m3_frame.phase[1].status == CURRENT_MISSING);
    adc_ready = true;
    tick(2560, 160, false, true);
    assert(g_m3_frame.valid_mask == 4 && g_m3_frame.pwm_cycle == 3 && g_m3_frame.sector == 1);

    tick(2290, -2351, true, true);
    assert(arms == 4);
    adc_ready = true;
    inject_stop_on_finish = true;
#ifdef BENCH_DEBUG_UART
    epoch = snapshot_epoch();
#endif
    tick(2511, 160, false, true);
    assert(stopped && !g_m3_running && !g_m3_frame_current && g_m3_frame_count == 3);
#ifdef BENCH_DEBUG_UART
    assert(snapshot_epoch() == epoch + 1u);
    bench_current_poll(); /* Held physical STOP must not invalidate again. */
    assert(snapshot_epoch() == epoch + 1u);
#endif
    inject_stop_on_finish = brake = false;
    apply(2);
    assert(g_m3_running && arms == 5);
    tick(4810, -2391, true, true);
    assert(stopped && !g_m3_running && g_m3_frame_count == 4);
    assert(g_m3_frame.valid_mask == 0 && g_m3_frame.phase[2].status == CURRENT_MISSING);
    apply(3);
    g_m3_requested_sector = 4;
    g_m3_command = BENCH_CURRENT_APPLY;
    adc_ready = true;
    tick(2560, 160, false, true);
    assert(g_m3_frame.valid_mask == 1 && g_m3_frame.sector == 3);
    assert(g_m3_frame.calibration_version == 7 && g_m3_running);
    adc_ready = true;
    tick(2560, 160, false, true);
    assert(g_m3_frame.valid_mask == 1 && g_m3_frame.sector == 4);
    assert(g_m3_frame_current);
    tick(2290, -2351, true, true);
    adc_ready = true;
    g_m3_requested_sector = 5;
    g_m3_command = BENCH_CURRENT_STOP;
    check_stop_order = true;
    tick(2511, 160, false, true);
    assert(!check_stop_order && stopped && !g_m3_running && !g_m3_frame_current);
    assert(g_m3_command == 0 && g_m3_frame.sector == 4 && g_m3_frame.valid_mask == 1);
    {
        uint32_t count = g_m3_frame_count;
        unsigned previous_arms = arms;
        adc_ready = true;
        tick(2200, -2351, true, true);
        assert(g_m3_frame_count == count && arms == previous_arms);
        restart_prepare_ticks = 4900;
        apply(5);
        assert(g_m3_running && !g_m3_frame_current && g_m3_frame_count == count);
        tick(22, -2378, false, false);
        assert(g_m3_running && g_m3_frame_count == count);
        adc_ready = true;
        tick(2538, 160, false, true);
        assert(g_m3_frame_count == count + 1 && g_m3_frame.sector == 5);
        assert(g_m3_frame.valid_mask == 2 && g_m3_frame_current);
        puts("PASS: restart preparation longer than a carrier does not time out first poll");
    }
    {
        uint32_t count = g_m3_frame_count;
        apply(0);
        tick(2560, 160, false, true);
        assert(g_m3_frame_count == count);
        adc_ready = true;
        tick(550, 710, false, true);
        assert(g_m3_frame_count == count + 1 && g_m3_frame.valid_mask == 2);
        assert(g_m3_pwm_debug.reason == BENCH_PWM_FINISH_ADC_READY);

        apply(0);
        tick(2560, 160, false, true);
        assert(g_m3_frame_count == count + 1);
        tick(640, 800, false, true);
        assert(g_m3_frame_count == count + 2 && g_m3_frame.phase[1].status == CURRENT_MISSING);
        assert(g_m3_pwm_debug.reason == BENCH_PWM_FINISH_ADC_TIMEOUT);

        apply(0);
        adc_ready = true;
        tick(3200, 800, false, true);
        assert(g_m3_frame_count == count + 3 && g_m3_frame.valid_mask == 0);
        assert(g_m3_pwm_debug.reason == BENCH_PWM_FINISH_ATTRIBUTION_LOST);
        puts("PASS: hardware completion can arrive late within a known cycle; deadline rejects it");
    }
    {
        uint32_t count = g_m3_frame_count;
        apply(0);
        adc_ready = true;
        finish_entry_ticks = 327;
        publish_delay_ticks = 40;
        tick(3169, 537, false, true);
        assert(g_m3_frame_count == count + 1 && g_m3_frame.raw_mask == 2);
        assert(g_m3_pwm_debug.reason == BENCH_PWM_FINISH_ADC_READY);
        assert(g_m3_pwm_debug.close_elapsed_ticks == 3169);

        finish_entry_ticks = 0;
        publish_delay_ticks = 0;
        close_delay_ticks = 31;
        apply(0);
        adc_ready = true;
        tick(3169, 537, false, true);
        assert(g_m3_frame_count == count + 2 && g_m3_frame.raw_mask == 0);
        assert(g_m3_pwm_debug.reason == BENCH_PWM_FINISH_ATTRIBUTION_LOST);
        assert(g_m3_pwm_debug.close_elapsed_ticks == 3200);
        puts("PASS: deadline applies to trigger closure, not frame publication");
    }
    assert(g_m3_running);
    {
        unsigned previous_arms = arms;
        uint32_t frames = g_m3_frame_count;

        g_m3_command = BENCH_CURRENT_STATIC;
        g_m3_requested_sector = 3;
        bench_current_poll();
        assert(g_m3_running && g_m3_frame_count == frames);
#ifdef BENCH_DEBUG_UART
        epoch = snapshot_epoch();
#endif
        g_m3_command = BENCH_CURRENT_STOP;
        bench_current_poll();
#ifdef BENCH_DEBUG_UART
        assert(snapshot_epoch() == epoch + 1u);
#endif
        g_m3_command = BENCH_CURRENT_STATIC;
        bench_current_poll();
#ifdef BENCH_DEBUG_UART
        assert(snapshot_epoch() == epoch + 2u);
#endif
        assert(stopped && !g_m3_running && arms == previous_arms);
        assert(g_m3_frame_count == frames + 1 && g_m3_frame.pwm_cycle == 0);
        assert(g_m3_frame.sector == 3 && g_m3_frame.raw_mask == 1);
        puts("PASS: software-triggered static sample leaves PWM outputs off");
    }
#ifdef BENCH_DEBUG_UART
    {
        unsigned i;
        uint32_t dropped = lks_uart_stats().tx_dropped;
        uint32_t frames = g_m3_frame_count;
        for (i = 0; i < 60; ++i) {
            tick(1, 0, false, false);
        }
        assert(lks_uart_stats().tx_dropped > dropped);
        assert(stopped && !g_m3_running && frames == g_m3_frame_count);
        puts("PASS: full UART TX drops telemetry and leaves stopped sample unchanged");
    }
#endif
    puts("PASS: incomplete and completed conversions retain old sector across restart");
    puts("PASS: physical STOP during finish and software STOP before finish suppress publication");
    puts("M3 Bench: first PWM cycle, wrap, commutation, duplicate event and STOP Mock PASS");
    return 0;
}
