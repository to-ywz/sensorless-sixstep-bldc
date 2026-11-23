#include <assert.h>
#include <stdio.h>
#include "lks_current_sample.h"
#include "lks32mc07x_lib.h"

ADC_TypeDef mock_adc;
uint32_t MCPWM0_PRT, MCPWM0_TMR0, MCPWM0_UPDATE, MCPWM0_IF0, MCPWM0_CNT0;
static uint8_t selected_channel;
static uint32_t data_reads;
static uint32_t bus_tick, clear_at;
static uint16_t adc_clear_mask;
static bool stuck_flag;

uint32_t bench_sixstep_clock(void)
{
    ++bus_tick;
    if (adc_clear_mask && !stuck_flag && bus_tick - clear_at >= 20) {
        mock_adc.IF &= ~adc_clear_mask;
        adc_clear_mask = 0;
    }
    return bus_tick;
}

void ADC_Init(ADC_TypeDef *adc, ADC_InitTypeDef *config)
{
    assert(config->unused == 0);
    adc->TRIG = 0;
}

void ADC_StructInit(ADC_InitTypeDef *config)
{
    config->unused = 0;
}

void ADC_ClearIRQFlag(ADC_TypeDef *adc, uint16_t flags)
{
    assert(adc == ADC0);
    adc_clear_mask = flags;
    clear_at = bus_tick;
}

void ADC_CHN_GAIN_CFG(ADC_TypeDef *adc, int slot, uint8_t channel, int gain)
{
    assert(adc->TRIG == 0 && slot == CHN0 && gain == ADC_GAIN3V6);
    selected_channel = channel;
}

int16_t ADC_GetConversionValue(ADC_TypeDef *adc, int slot)
{
    assert(adc->TRIG == 0 && slot == DAT0);
    ++data_reads;
    return 0x0640;
}

static void test_portable(void)
{
    CurrentCalibration cal = {18, {{10, 50}, {-5, 20}, {0, -10}}};
    uint16_t words[] = {0x0640, 0xfe20, 0x01e0};
    CurrentTrigger trigger = current_trigger(UINT32_MAX, 1, true, true, &cal);
    CurrentSampleFrame frame;
    int code;

    for (code = -2048; code < 2048; ++code) {
        assert(current_decode_adc((uint16_t)(code * 16), false) == code);
        assert(current_decode_adc((uint16_t)code, true) == code);
    }
    cal.version = 19;
    cal.phase[0].microamps_per_code = 1;
    frame = current_sample(&trigger, words, 7, false, 0, 1);
    assert(frame.valid_mask == 7 && frame.raw_mask == 7 && frame.calibration_version == 18);
    assert(frame.pwm_cycle == UINT32_MAX && frame.sector == 1);
    assert(frame.phase[0].raw == 100 && frame.phase[0].microamps == 4500);
    assert(frame.phase[1].raw == -30 && frame.phase[1].microamps == -500);
    assert(frame.phase[2].raw == 30 && frame.phase[2].microamps == -300);

    frame = current_sample(&trigger, words, 7, false, 1, 1);
    assert(frame.valid_mask == 0 && frame.phase[0].status == CURRENT_EXPIRED);
    words[0] = 0x8000;
    words[1] = 0x7ff0;
    frame = current_sample(&trigger, words, 3, false, 0, 1);
    assert(frame.valid_mask == 0 && frame.phase[0].status == CURRENT_SATURATED);
    assert(frame.phase[1].status == CURRENT_SATURATED && frame.phase[2].status == CURRENT_MISSING);
    trigger = current_trigger(1, 6, true, true, &cal);
    frame = current_sample(&trigger, words, 7, false, 1, 0);
    assert(frame.valid_mask == 0 && frame.phase[0].status == CURRENT_BAD_SECTOR);
    cal.version = 0;
    trigger = current_trigger(1, 0, true, true, &cal);
    frame = current_sample(&trigger, words, 7, false, 1, 0);
    assert(frame.valid_mask == 0 && frame.phase[2].status == CURRENT_UNCALIBRATED);
}

static void test_pwm_adapter(void)
{
    const CurrentCalibration cal = {1, {{0, 1}, {0, 2}, {0, 3}}};
    const uint8_t sink[] = {1, 2, 2, 0, 0, 1};
    LksCurrentSampler sampler;
    CurrentSampleFrame frame;
    uint8_t sector;

    lks_current_init(&sampler, -100);
    assert(MCPWM0_TMR0 == (uint16_t)-100 && MCPWM0_PRT == 0xcafe);
    assert(MCPWM0_UPDATE == MCPWM0_UPDATE_TMR0_UPDATE_BIT);
    for (sector = 0; sector < 6; ++sector) {
        assert(lks_current_arm(&sampler, sector, sector, true, &cal));
        assert(mock_adc.TRIG == ADC_TRIG_MCPWM0_T0 && selected_channel == 2 - sink[sector]);
        assert(!lks_current_close_if_ready(&sampler));
        assert(mock_adc.TRIG == ADC_TRIG_MCPWM0_T0);
        mock_adc.IF = ADC_SF1_IF;
        assert(lks_current_close_if_ready(&sampler));
        assert(mock_adc.TRIG == 0 && sampler.trigger_closed_at == bus_tick);
        assert(!sampler.boundary_at_close);
        bus_tick += 327;
        assert(!lks_current_arm(&sampler, sector + 1, (sector + 1) % 6, true, &cal));
        assert(lks_current_finish(&sampler, sector + 1, true, &frame));
        assert(sampler.trigger_closed_at == bus_tick - 327);
        assert(frame.sector == sector && frame.pwm_cycle == sector);
        assert(frame.valid_mask == (1u << sink[sector]));
        assert(frame.phase[sink[sector]].microamps == 100 * (sink[sector] + 1));
        assert(frame.phase[(sink[sector] + 1) % 3].status == CURRENT_NOT_SELECTED);
    }
    assert(!lks_current_finish(&sampler, 6, true, &frame));
    assert(lks_current_arm(&sampler, 6, 0, false, &cal));
    assert(mock_adc.TRIG == 0 && lks_current_finish(&sampler, 6, true, &frame));
    assert(frame.phase[1].status == CURRENT_OUTSIDE_WINDOW);
    assert(lks_current_arm(&sampler, 7, 0, true, &cal));
    assert(lks_current_finish(&sampler, 7, true, &frame));
    assert(frame.phase[1].status == CURRENT_MISSING);
    mock_adc.IF = ADC_SF1_IF;
    assert(!lks_current_arm(&sampler, 8, 1, true, &cal));

    lks_current_init(&sampler, 100);
    assert(lks_current_arm(&sampler, 9, 1, true, &cal));
    mock_adc.IF = ADC_SF1_IF | ADC_HERR_IF;
    assert(lks_current_finish(&sampler, 9, true, &frame));
    assert(frame.phase[2].status == CURRENT_MISSING);
    lks_current_init(&sampler, 100);
    assert(lks_current_arm(&sampler, 10, 1, true, &cal));
    mock_adc.IF = ADC_SF1_IF;
    assert(lks_current_finish(&sampler, 11, false, &frame));
    assert(frame.phase[2].status == CURRENT_MISSING && data_reads == 6);
    assert(g_m3_adc_debug.finish_count == 1 && g_m3_adc_debug.pwm_cycle == 10);
    assert(g_m3_adc_debug.sector == 1 && !g_m3_adc_debug.attributed);
    assert(g_m3_adc_debug.adc_flags == ADC_SF1_IF && g_m3_adc_debug.adc_idle);

    lks_current_init(&sampler, 0);
    mock_adc.IF = ADC_SF1_IF;
    assert(lks_current_arm(&sampler, 12, 1, true, &cal));
    assert(mock_adc.IF == 0 && bus_tick - clear_at >= 20);
    assert(lks_current_finish(&sampler, 12, true, &frame));
    assert(frame.phase[2].status == CURRENT_MISSING && data_reads == 6);

    mock_adc.IF = ADC_SF1_IF;
    stuck_flag = true;
    assert(!lks_current_recover(&sampler));
    assert(!lks_current_arm(&sampler, 13, 1, true, &cal) && mock_adc.TRIG == 0);
    stuck_flag = false;
    mock_adc.CFG = ADCx_CFG_FSM_RS_BIT;
    assert(!lks_current_recover(&sampler));
    mock_adc.CFG = 0;
    assert(lks_current_recover(&sampler));
    assert(lks_current_arm(&sampler, 14, 1, true, &cal));
    lks_current_cancel(&sampler);
    mock_adc.IF = ADC_SF1_IF;
    assert(!lks_current_finish(&sampler, 14, true, &frame));
    assert(data_reads == 6 && mock_adc.TRIG == 0);
}

int main(void)
{
    test_portable();
    test_pwm_adapter();
    puts("M3 current sampling: ADC decode, calibrated frames and six-sector MCPWM adapter Mock PASS");
    return 0;
}
