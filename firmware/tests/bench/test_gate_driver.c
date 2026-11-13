/* A5 gate checkout: motor disconnected, P3 removed, POWER BUS MUST BE 0V.
 * STOP is polled software control, not a hardware power-stage protection.
 */
#include "lks32mc07x_lib.h"
#include "bench_sixstep_output.h"
#include "test_gate_driver.h"

volatile uint32_t g_a5_phase, g_a5_test, g_a5_command, g_a5_bootstrap_checked;  // 仿真修改数值
volatile uint32_t g_a5_state, g_a5_fault, g_a5_completed;
static uint32_t active_phase, active_test, active_checked, entered_at;
static uint32_t candidate, stable, changed_at;
static uint8_t start_armed;
enum { OFF=0, LOW_HOLD, PRECHARGE, ALL_OFF_GAP, HIGH_PACKET };
enum { LOW_HOLD_US=5000000u, PRECHARGE_US=1000u, GAP_US=50u, PACKET_US=200u };

static void stop(uint32_t fault)
{
    bench_sixstep_output_stop();
    g_a5_state=OFF;
    g_a5_fault=fault;
}
static int output(uint32_t high)
{
    if (!bench_gate_output_apply(active_phase, high)) {
        stop(3u); return 0;
    }
    entered_at=bench_sixstep_clock();
    return 1;
}
void bench_gate_driver_init(void)
{

    GPIO_InitTypeDef gpio;
    GPIO_PinAFConfig(GPIO2,GPIO_PinSource_11,AF0_GPIO);
    GPIO_PinAFConfig(GPIO2,GPIO_PinSource_12,AF0_GPIO);
    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin=GPIO_Pin_11|GPIO_Pin_12;
    gpio.GPIO_Mode=GPIO_Mode_IN;
    gpio.GPIO_PuPd=GPIO_PuPd_NOPULL;
    GPIO_Init(GPIO2,&gpio);     // 按键初始化，一个用于启动，一个用于停止

    bench_sixstep_output_init(); // 初始化六路PWM输出
    g_a5_phase=g_a5_test=g_a5_command=g_a5_bootstrap_checked=0u;
    g_a5_state=g_a5_fault=g_a5_completed=0u;


    candidate=stable=(GPIO_ReadInputDataBit(GPIO2,GPIO_Pin_11)==0u);
    start_armed=(stable==0u);
    changed_at=bench_sixstep_clock(); // 按键事件检测
}
void bench_gate_driver_poll(void)
{
    uint32_t now=bench_sixstep_clock(), ms=bench_sixstep_ticks_per_ms();
    uint32_t us=ms/1000u, elapsed, limit, command=g_a5_command;
    uint32_t pressed=(GPIO_ReadInputDataBit(GPIO2,GPIO_Pin_11)==0u);
    uint8_t event=0u;
    g_a5_command=0u;

    if (candidate!=pressed) {candidate=pressed;changed_at=now;}
    if (candidate!=stable && (uint32_t)(now-changed_at)>=20u*ms) {
        stable=candidate;
        if (!stable) start_armed=1u;
        else if (start_armed) {event=1u;start_armed=0u;}
    }
    if (GPIO_ReadInputDataBit(GPIO2,GPIO_Pin_12)==0u || command==2u) {
        stop(0u);start_armed=0u;stable=1u;changed_at=now;return;
    }// 按键检测

    if (command>2u || us==0u || g_a5_state>HIGH_PACKET || g_a5_phase>2u || g_a5_test>1u ||
        g_a5_bootstrap_checked>7u) {stop(1u);return;} 
    if (g_a5_state!=OFF && (active_phase!=g_a5_phase || active_test!=g_a5_test ||
        active_checked!=g_a5_bootstrap_checked)) {stop(4u);return;} // 检测仿真修改数值是否异常
    if (g_a5_state==OFF) {
        if (!(event || command==1u)) return;
        if (g_a5_test==1u && !(g_a5_bootstrap_checked&(1u<<g_a5_phase))) {
            stop(2u);return;
        } 
        active_phase=g_a5_phase;active_test=g_a5_test;
        active_checked=g_a5_bootstrap_checked;g_a5_fault=0u;
        if (output(0u)) g_a5_state=active_test?PRECHARGE:LOW_HOLD;
        return;
    } // 检测到按键事件或仿真修改数值, 开始输出PWM信号
    /* RUN while active is ignored: it cannot restart/extend a pulse packet. */
    elapsed=(uint32_t)(now-entered_at);
    limit=(g_a5_state==LOW_HOLD?LOW_HOLD_US:
           g_a5_state==PRECHARGE?PRECHARGE_US:
           g_a5_state==ALL_OFF_GAP?GAP_US:PACKET_US)*us; // 该状态需要持续时间
    
    if (elapsed<limit) return;
    
    if (g_a5_state==LOW_HOLD || g_a5_state==HIGH_PACKET) {
        stop(0u);++g_a5_completed;return;
    } // 检测到PWM输出时间已到, 停止输出

    /* Missed precharge/gap deadline by >1 ms: stop instead of issuing a
     * delayed high packet with unknown bootstrap charge. Not an HW timeout.
     */
    if (elapsed-limit>ms) {stop(5u);return;} // 软件超时保护，防止单个状态轮询时等待超时
    if (g_a5_state==PRECHARGE) {
        bench_sixstep_output_stop();
        entered_at=bench_sixstep_clock();g_a5_state=ALL_OFF_GAP; // 预充完毕
    } else if (g_a5_state==ALL_OFF_GAP) {
        if (output(1u)) g_a5_state=HIGH_PACKET; // 间隔时间完毕，准备脉冲
    } else stop(1u); // 
}
  