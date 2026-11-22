#include <assert.h>
#include <stdio.h>
#include "../../firmware/tests/bench_current/bench_current.h"
#include "../../firmware/platform/lks32mc071/bench_sixstep_output.h"
#include "../../firmware/platform/lks32mc071/lks_current_sample.h"

static uint32_t clock_ticks;
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

void lks_current_init(LksCurrentSampler *sampler, int16_t compare_tick)
{
    assert(compare_tick == 0);
    *sampler = (LksCurrentSampler){0};
}
LksCurrentPwmEvent lks_current_pwm_event(void) { return event; }
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
    ++arms;
    return true;
}

bool lks_current_finish(LksCurrentSampler *sampler, uint32_t cycle,
                        bool attributed, CurrentSampleFrame *frame)
{
    const uint16_t words[CURRENT_PHASES] = {0x0800, 0x0800, 0x0800};
    uint8_t mask = adc_ready && attributed ? sampler->trigger.phase_mask : 0;
    assert(sampler->pending);
    *frame = current_sample(&sampler->trigger, words, mask, false, cycle, 1);
    sampler->pending = false;
    sampler->needs_idle_init = !mask;
    adc_ready = false;
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
}

int main(void)
{
    const CurrentCalibration cal = {7, {{0, 1}, {0, 1}, {0, 1}}};
    clock_ticks = UINT32_MAX - 100;
    bench_current_init(&cal);
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
    tick(10, 170, false, true);
    assert(g_m3_frame_count == 1 && arms == 1);

    tick(2280, -2351, true, true);
    assert(arms == 2);
    tick(50, -2301, false, false);
    startup_boundary = false;
    apply(1);
    assert(arms == 3 && g_m3_frame.sector == 0 && g_m3_frame.pwm_cycle == 2);
    assert(g_m3_frame.valid_mask == 0 && g_m3_frame.phase[1].status == CURRENT_MISSING);
    adc_ready = true;
    tick(2560, 160, false, true);
    assert(g_m3_frame.valid_mask == 4 && g_m3_frame.pwm_cycle == 3 && g_m3_frame.sector == 1);

    tick(2290, -2351, true, true);
    assert(arms == 4);
    adc_ready = true;
    inject_stop_on_finish = true;
    tick(2511, 160, false, true);
    assert(stopped && !g_m3_running && !g_m3_frame_current && g_m3_frame_count == 3);
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
        apply(5);
        assert(g_m3_running && !g_m3_frame_current && g_m3_frame_count == count);
    }
    puts("PASS: incomplete and completed conversions retain old sector across restart");
    puts("PASS: physical STOP during finish and software STOP before finish suppress publication");
    puts("M3 Bench: first PWM cycle, wrap, commutation, duplicate event and STOP Mock PASS");
    return 0;
}
