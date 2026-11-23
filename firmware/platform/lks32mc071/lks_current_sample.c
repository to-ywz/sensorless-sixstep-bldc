#include "lks_current_sample.h"
#include "lks32mc07x_lib.h"
#include "bench_sixstep_output.h"

#define SAMPLE_ADC_FLAGS (ADC_SF1_IF | ADC_HERR_IF | ADC_SERR_IF)
#define ADC_CLEAR_BUS_CYCLES 20u

volatile LksCurrentDebug g_m3_adc_debug;
#define PWM_SAMPLE_FLAGS (MCPWM0_IF0_T0_IF_BIT | MCPWM0_IF0_TMR0_IF_BIT)

static bool adc_idle(void)
{
    return (ADC0->CFG & ADCx_CFG_FSM_RS_BIT) == 0u;
}

static void close_trigger(LksCurrentSampler *sampler)
{
    ADC0->TRIG = 0;
    sampler->trigger_closed_at = bench_sixstep_clock();
    sampler->boundary_at_close = (MCPWM0_IF0 & MCPWM0_IF0_T0_IF_BIT) != 0u;
    sampler->trigger_closed = true;
}

bool lks_current_close_if_ready(LksCurrentSampler *sampler)
{
    uint32_t flags = ADC0->IF;
    if ((flags & (ADC_HERR_IF | ADC_SERR_IF)) == 0u &&
        ((flags & ADC_SF1_IF) == 0u || !adc_idle())) {
        return false;
    }
    close_trigger(sampler);
    return true;
}

static bool clear_adc_flags(void)
{
    uint32_t start;
    ADC_ClearIRQFlag(ADC0, SAMPLE_ADC_FLAGS);
    start = bench_sixstep_clock();
    /* The MCU071 bench uses a core-clock SysTick and undivided 48 MHz bus.
     * Wait through the ADC_IF clock-domain crossing before rearming.
     */
    while ((uint32_t)(bench_sixstep_clock() - start) < ADC_CLEAR_BUS_CYCLES) {}
    return adc_idle() && (ADC0->IF & SAMPLE_ADC_FLAGS) == 0u;
}

void lks_current_cancel(LksCurrentSampler *sampler)
{
    ADC0->TRIG = 0;
    sampler->pending = false;
    sampler->needs_idle_init = true;
}

bool lks_current_recover(LksCurrentSampler *sampler)
{
    if (sampler->pending) {
        return false;
    }
    ADC0->TRIG = 0;
    if (!adc_idle()) {
        ADC0->CFG |= ADCx_CFG_FSM_RS_BIT;
        return false;
    }
    sampler->needs_idle_init = !clear_adc_flags();
    return !sampler->needs_idle_init;
}

LksCurrentPwmEvent lks_current_pwm_event(void)
{
    LksCurrentPwmEvent event;
    uint32_t flags = MCPWM0_IF0;
    int32_t tick = MCPWM0_CNT0 & 0xffffu;
    event.tick = (int16_t)(tick >= 32768 ? tick - 65536 : tick);
    event.boundary = (flags & MCPWM0_IF0_T0_IF_BIT) != 0u;
    event.compare = (flags & MCPWM0_IF0_TMR0_IF_BIT) != 0u;
    return event;
}

void lks_current_clear_pwm_events(void)
{
    MCPWM0_IF0 = PWM_SAMPLE_FLAGS;
}

void lks_current_init(LksCurrentSampler *sampler, int16_t compare_tick)
{
    ADC_InitTypeDef config = {0};

    g_m3_adc_debug = (LksCurrentDebug){0};
    ADC_StructInit(&config);
    ADC_Init(ADC0, &config);
    sampler->pending = false;
    sampler->needs_idle_init = !clear_adc_flags();

    MCPWM0_PRT = 0xDEADu;
    MCPWM0_TMR0 = (uint16_t)compare_tick;
    MCPWM0_UPDATE = MCPWM0_UPDATE_TMR0_UPDATE_BIT;
    MCPWM0_PRT = 0xCAFEu;

}

bool lks_current_arm(LksCurrentSampler *sampler, uint32_t cycle, uint8_t sector,
                     bool in_window, const CurrentCalibration *calibration)
{
    static const uint8_t channels[CURRENT_PHASES] = {
        ADC_CHANNEL_2, ADC_CHANNEL_1, ADC_CHANNEL_0
    };
    uint8_t phase;

    if (sampler->pending || sampler->needs_idle_init) {
        return false;
    }

    sampler->trigger = current_trigger(cycle, sector, in_window, false, calibration);
    sampler->pending = true;
    sampler->trigger_closed = false;
    if (!in_window) {
        return true;
    }
    for (phase = 0; phase < CURRENT_PHASES; ++phase) {
        if (sampler->trigger.phase_mask & (1u << phase)) {
            if (!adc_idle() || !clear_adc_flags()) {
                sampler->pending = false;
                sampler->needs_idle_init = true;
                return false;
            }
            ADC_CHN_GAIN_CFG(ADC0, CHN0, channels[phase], ADC_GAIN3V6);
            ADC0->TRIG = ADC_TRIG_MCPWM0_T0;
        }
    }
    return true;
}

bool lks_current_finish(LksCurrentSampler *sampler, uint32_t now_cycle,
                        bool before_next_compare, CurrentSampleFrame *frame)
{
    uint16_t words[CURRENT_PHASES] = {0};
    uint8_t received = 0;

    if (!sampler->pending) {
        return false;
    }
    if (sampler->trigger.in_window && sampler->trigger.phase_mask != 0) {
        uint32_t flags;
        uint8_t phase;
        bool idle;

        if (!sampler->trigger_closed) {
            close_trigger(sampler);
        }
        flags = ADC0->IF;
        idle = adc_idle();
        if (before_next_compare && idle && (flags & SAMPLE_ADC_FLAGS) == ADC_SF1_IF) {
            for (phase = 0; phase < CURRENT_PHASES; ++phase) {
                if (sampler->trigger.phase_mask & (1u << phase)) {
                    words[phase] = (uint16_t)ADC_GetConversionValue(ADC0, DAT0);
                }
            }
            received = sampler->trigger.phase_mask;
        } else {
            /* An unfinished conversion could still publish after the trigger is masked. */
            sampler->needs_idle_init = true;
        }
        ADC_ClearIRQFlag(ADC0, SAMPLE_ADC_FLAGS);
        g_m3_adc_debug.pwm_cycle = sampler->trigger.pwm_cycle;
        g_m3_adc_debug.sector = sampler->trigger.sector;
        g_m3_adc_debug.attributed = before_next_compare;
        g_m3_adc_debug.adc_flags = flags;
        g_m3_adc_debug.adc_idle = idle;
        ++g_m3_adc_debug.finish_count;
    }
    *frame = current_sample(&sampler->trigger, words, received, false, now_cycle, 1u);
    sampler->pending = false;
    return true;
}
