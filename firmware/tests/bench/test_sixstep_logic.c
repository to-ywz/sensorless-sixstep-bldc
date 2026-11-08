#include "lks32mc07x_lib.h"
#include "sixstep_table.h"
#include "bench_sixstep_output.h"
#include "test_sixstep_logic.h"

volatile uint32_t g_a4_mode, g_a4_reverse, g_a4_duty_permille=250u;
volatile uint32_t g_a4_command, g_a4_test_sector;
volatile uint32_t g_a4_sector=255u, g_a4_running, g_a4_fault, g_a4_transitions;
static uint32_t active_mode, active_reverse, active_duty;
static uint32_t candidate, stable, changed_at, sector_at;
static uint8_t start_armed;

static void stop(void)
{
    bench_sixstep_output_stop();
    g_a4_running=0u;
    g_a4_sector=255u;
}
static int apply(uint32_t sector)
{
    if (!bench_sixstep_output_apply(sector,active_duty)) {
        stop(); g_a4_fault=1u; return 0;
    }
    g_a4_sector=sector;
    ++g_a4_transitions;
    sector_at=bench_sixstep_clock();
    return 1;
}
void bench_sixstep_logic_init(void)
{
    GPIO_InitTypeDef gpio;
    GPIO_PinAFConfig(GPIO2,GPIO_PinSource_11,AF0_GPIO);
    GPIO_PinAFConfig(GPIO2,GPIO_PinSource_12,AF0_GPIO);
    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin=GPIO_Pin_11 | GPIO_Pin_12;
    gpio.GPIO_Mode=GPIO_Mode_IN;
    gpio.GPIO_PuPd=GPIO_PuPd_NOPULL;
    GPIO_Init(GPIO2,&gpio);
    bench_sixstep_output_init();
    candidate=stable=(GPIO_ReadInputDataBit(GPIO2,GPIO_Pin_11)==0u);
    changed_at=bench_sixstep_clock();
    start_armed=(stable==0u);
    g_a4_command=0u;
    g_a4_running=g_a4_fault=g_a4_transitions=0u;
    g_a4_sector=255u;
}
void bench_sixstep_logic_poll(void)
{
    uint32_t now=bench_sixstep_clock(), ms=bench_sixstep_ticks_per_ms();
    uint32_t pressed=(GPIO_ReadInputDataBit(GPIO2,GPIO_Pin_11)==0u);
    uint32_t command=g_a4_command;
    uint8_t event=0u, next;
    g_a4_command=0u;
    if (candidate!=pressed) { candidate=pressed; changed_at=now; }
    if (stable!=candidate && (uint32_t)(now-changed_at)>=20u*ms) {
        stable=candidate;
        if (!stable) start_armed=1u;
        else if (start_armed) { event=1u; start_armed=0u; }
    }
    /* STOP always wins; consume START and require a stable release afterwards. */
    if (GPIO_ReadInputDataBit(GPIO2,GPIO_Pin_12)==0u || command==2u) {
        stop(); start_armed=0u; changed_at=now; stable=1u; return;
    }
    if (g_a4_running && (g_a4_mode!=active_mode ||
        g_a4_reverse!=active_reverse || g_a4_duty_permille!=active_duty)) {
        stop(); return; /* Configuration changes require a separate restart. */
    }
    if (command>3u) { stop(); g_a4_fault=2u; return; }
    if (command==3u) {
        if (g_a4_running) (void)apply(g_a4_test_sector);
        return;
    }
    if (event || command==1u) {
        if (!g_a4_running) {
            if (g_a4_mode>1u || g_a4_reverse>1u ||
                !sixstep_duty_valid(g_a4_duty_permille)) {
                stop(); g_a4_fault=2u; return;
            }
            active_mode=g_a4_mode; active_reverse=g_a4_reverse;
            active_duty=g_a4_duty_permille; g_a4_fault=0u;
            if (apply(0u)) g_a4_running=1u;
        } else if (!active_mode) {
            if (sixstep_next(g_a4_sector,active_reverse,&next)) (void)apply(next);
            else { stop(); g_a4_fault=2u; }
        }
        return;
    }
    if (g_a4_running && active_mode && (uint32_t)(now-sector_at)>=100u*ms) {
        if (sixstep_next(g_a4_sector,active_reverse,&next)) (void)apply(next);
        else { stop(); g_a4_fault=2u; }
    }
}
