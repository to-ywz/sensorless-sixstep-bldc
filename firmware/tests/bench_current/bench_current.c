#include "bench_current.h"
#include "../../platform/lks32mc071/bench_sixstep_output.h"
#include "../../platform/lks32mc071/lks_current_sample.h"

enum {
    BENCH_DUTY = 250,
    SAMPLE_COMPARE = 0,
    ARM_LATEST = -512,
    FINISH_EARLIEST = 128,
    FINISH_LATEST = 512,
    /* Shared A4 carrier is 4801 ticks at the same 48 MHz as SysTick.
     * This bound includes arm preparation and is strictly below one carrier.
     */
    ATTRIBUTION_TICKS = 3200,
    CARRIER_TICKS = 2 * BENCH_SIXSTEP_HALF_PERIOD_TICKS + 1
};

volatile uint32_t g_m3_command, g_m3_requested_sector;
volatile uint32_t g_m3_running, g_m3_frame_count, g_m3_timing_faults;
volatile CurrentSampleFrame g_m3_frame;
volatile bool g_m3_frame_current;
volatile BenchCurrentPwmDebug g_m3_pwm_debug;

static LksCurrentSampler sampler;
/* Until a board calibration exists, publish raw data with UNCALIBRATED status. */
static CurrentCalibration calibration;
static uint32_t cycle, armed_at, polled_at;
static uint8_t sector, requested_sector;
static bool restarted;

static void stop(void)
{
    bench_sixstep_output_stop();
    lks_current_cancel(&sampler);
    g_m3_running = 0;
    g_m3_frame_current = false;
}

static void publish_missing(void)
{
    CurrentTrigger trigger = current_trigger(cycle, sector, true, false, &calibration);
    const uint16_t empty[CURRENT_PHASES] = {0};

    g_m3_frame = current_sample(&trigger, empty, 0, false, cycle, 0);
    ++g_m3_frame_count;
    g_m3_frame_current = true;
    ++g_m3_timing_faults;
}

static void prepare_restart(void)
{
    lks_current_clear_pwm_events();
    sector = requested_sector;
    ++cycle;
    restarted = true;
    if (sampler.needs_idle_init) {
        lks_current_recover(&sampler);
    }
    /* The counter is stopped. A previous conversion must not own this trigger. */
    armed_at = bench_sixstep_clock();
    (void)lks_current_arm(&sampler, cycle, sector, true, &calibration);
}

static void finish_period(bool attributed)
{
    CurrentSampleFrame frame;
    if (!lks_current_finish(&sampler, cycle, attributed, &frame)) {
        return;
    }
    if (bench_sixstep_stop_requested() || g_m3_command == BENCH_CURRENT_STOP) {
        stop();
        return;
    }
    if ((uint32_t)(bench_sixstep_clock() - armed_at) >= ATTRIBUTION_TICKS || !attributed) {
        const uint16_t empty[CURRENT_PHASES] = {0};
        frame = current_sample(&sampler.trigger, empty, 0, false, cycle, 0);
        lks_current_cancel(&sampler);
    }
    if (!frame.raw_mask) {
        ++g_m3_timing_faults;
    }
    g_m3_frame = frame;
    ++g_m3_frame_count;
    g_m3_frame_current = true;
}

void bench_current_init(const CurrentCalibration *board_calibration)
{
    bench_sixstep_stop_input_init();
    bench_sixstep_output_init();
    lks_current_init(&sampler, SAMPLE_COMPARE);
    calibration = board_calibration ? *board_calibration : (CurrentCalibration){0};
    cycle = 0;
    restarted = false;
    g_m3_command = g_m3_requested_sector = 0;
    g_m3_running = g_m3_frame_count = g_m3_timing_faults = 0;
    g_m3_frame = (CurrentSampleFrame){0};
    g_m3_pwm_debug = (BenchCurrentPwmDebug){0};
    g_m3_frame_current = false;
}

void bench_current_poll(void)
{
    uint32_t command = g_m3_command;
    uint32_t request = g_m3_requested_sector;
    uint32_t now = bench_sixstep_clock();
    LksCurrentPwmEvent event;

    /* Single writer: debugger writes only at the poll entry breakpoint. */
    g_m3_command = 0;
    if (command == BENCH_CURRENT_STOP || bench_sixstep_stop_requested()) {
        stop();
        return;
    }

    if (g_m3_running) {
        if ((uint32_t)(now - polled_at) >= CARRIER_TICKS) {
            /* A sticky PWM flag cannot tell us how many boundaries elapsed. */
            if (sampler.pending) {
                finish_period(false);
            } else {
                ++g_m3_timing_faults;
            }
            stop();
            return;
        }
        event = lks_current_pwm_event();
        /* A T0 flag near the start of a staged restart belongs to the
         * seeded period. A real next boundary cannot occur this early.
         */
        if (restarted) {
            bool early = !event.compare && event.tick < ARM_LATEST &&
                         (uint32_t)(now - armed_at) < 1024u;
            restarted = early && !event.boundary;
            if (early && event.boundary) {
                lks_current_clear_pwm_events();
                event.boundary = false;
            }
        }
        if (sampler.pending) {
            uint32_t elapsed = bench_sixstep_clock() - armed_at;
            bool timely = elapsed < ATTRIBUTION_TICKS;
            if (!timely || event.boundary || event.tick >= FINISH_EARLIEST) {
                uint32_t sample_cycle = sampler.trigger.pwm_cycle;
                finish_period(timely && !event.boundary && event.compare &&
                              event.tick >= FINISH_EARLIEST && event.tick <= FINISH_LATEST);
                g_m3_pwm_debug.pwm_cycle = sample_cycle;
                g_m3_pwm_debug.elapsed_ticks = elapsed;
                g_m3_pwm_debug.counter_tick = event.tick;
                g_m3_pwm_debug.boundary = event.boundary;
                g_m3_pwm_debug.compare = event.compare;
                ++g_m3_pwm_debug.finish_count;
            }
        }
        if (!g_m3_running) {
            return;
        }
    }
    polled_at = now;
    if (command == BENCH_CURRENT_APPLY) {
        if (request >= 6u) {
            stop();
            return;
        }
        /* Pending results without a confirmed finish cannot survive counter reset. */
        if (sampler.pending) {
            finish_period(false);
            if (!g_m3_running) {
                return;
            }
        }
        g_m3_frame_current = false;
        requested_sector = (uint8_t)request;
        if (!bench_sixstep_output_apply_synced(request, BENCH_DUTY, prepare_restart)) {
            stop();
        } else {
            /* Start the poll interval after the backend restarts CNT0. */
            polled_at = bench_sixstep_clock();
            g_m3_running = 1;
            if (!sampler.pending) {
                publish_missing();
            }
        }
        return;
    }
    if (!g_m3_running || !event.boundary) {
        return;
    }

    lks_current_clear_pwm_events();
    ++cycle;
    if (event.tick > ARM_LATEST) {
        publish_missing();
        return;
    }
    if (sampler.needs_idle_init) {
        lks_current_recover(&sampler);
    }
    armed_at = now;
    if (!lks_current_arm(&sampler, cycle, sector, true, &calibration)) {
        publish_missing();
    }
}
