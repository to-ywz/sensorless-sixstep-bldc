#include <assert.h>
#include <stdio.h>
#include "lks32mc07x_lib.h"
#include "bench_sixstep_output.h"
#include "test_gate_driver.h"
static uint32_t now, start_pressed, stop_pressed, phase=255u, high, reject;
void GPIO_PinAFConfig(uint32_t a,uint32_t b,uint32_t c){(void)a;(void)b;(void)c;}
void GPIO_StructInit(GPIO_InitTypeDef *p){p->GPIO_Pin=p->GPIO_Mode=p->GPIO_PuPd=0;}
void GPIO_Init(uint32_t a,GPIO_InitTypeDef *p){(void)a;(void)p;}
uint32_t GPIO_ReadInputDataBit(uint32_t a,uint32_t p)
{(void)a;return !(p==GPIO_Pin_11?start_pressed:stop_pressed);}
uint32_t bench_sixstep_clock(void){return now;}
uint32_t bench_sixstep_ticks_per_ms(void){return 1000u;}
void bench_sixstep_output_stop(void){phase=255u;high=0u;}
void bench_sixstep_output_init(void){bench_sixstep_output_stop();}
int bench_gate_output_apply(uint32_t p,uint32_t h)
{
    bench_sixstep_output_stop();
    if(reject||stop_pressed||p>2u||h>1u)return 0;
    phase=p;high=h;return 1;
}
static void poll(uint32_t us){now+=us;bench_gate_driver_poll();}
static void command(uint32_t c){g_a5_command=c;poll(0);}
static void init(uint32_t held)
{now=0u;start_pressed=held;stop_pressed=reject=0u;bench_gate_driver_init();}
static void button(uint32_t pressed)
{start_pressed=pressed;poll(0);poll(20000u);}
static void assert_off(void){assert(g_a5_state==0u&&phase==255u);}
static void start_high(uint32_t p)
{g_a5_phase=p;g_a5_test=1u;g_a5_bootstrap_checked=1u<<p;command(1u);}
int main(void)
{
    unsigned p,s;
    init(1);poll(30000);assert_off();button(0);button(1);
    assert(g_a5_state==1&&phase==0&&!high);
    poll(100000);assert(g_a5_state==1); /* holding START does not repeat */
    command(2);assert_off();
    for(p=0;p<3;p++) {
        init(0);g_a5_phase=p;command(1);assert(phase==p&&!high);
        poll(4999999);assert(g_a5_state==1);poll(1);assert_off();
        assert(g_a5_completed==1);poll(6000000);assert_off();
        init(0);g_a5_phase=p;g_a5_test=1;command(1);assert_off();
        assert(g_a5_fault==2); /* high test cannot bypass bootstrap setup */
        start_high(p);assert(g_a5_state==2&&phase==p&&!high);
        poll(999);assert(g_a5_state==2);poll(1);assert(g_a5_state==3&&phase==255);
        poll(49);assert(g_a5_state==3);poll(1);assert(g_a5_state==4&&phase==p&&high);
        poll(100);command(1);assert(g_a5_state==4); /* RUN never extends packet */
        poll(99);assert(g_a5_state==4);poll(1);assert_off();
        assert(g_a5_completed==1);
    }
    /* Physical STOP wins in every stage; release never auto-restarts. */
    for(s=1;s<=4;s++) {
        init(0);
        if(s==1)command(1);else start_high(0);
        if(s>=3) { poll(1000); }
        if(s==4) { poll(50); }
        assert(g_a5_state==s);stop_pressed=1;command(1);assert_off();
        stop_pressed=0;poll(100000);assert_off();
    }
    init(0);command(1);g_a5_phase=1;poll(0);assert_off();assert(g_a5_fault==4);
    command(1);assert(phase==1);g_a5_bootstrap_checked=2;poll(0);assert_off();
    init(0);g_a5_phase=3;command(1);assert_off();assert(g_a5_fault==1);
    init(0);g_a5_test=2;command(1);assert_off();assert(g_a5_fault==1);
    init(0);g_a5_bootstrap_checked=8;command(1);assert_off();assert(g_a5_fault==1);
    init(0);command(99);assert_off();assert(g_a5_fault==1);
    init(0);g_a5_state=99;poll(0);assert_off();assert(g_a5_fault==1);
    init(0);reject=1;command(1);assert_off();assert(g_a5_fault==3);
    init(0);start_high(0);poll(2001);assert_off();assert(g_a5_fault==5);
    init(0);start_high(0);poll(1000);poll(1051);assert_off();assert(g_a5_fault==5);
    init(0);now=0xffffff00u;start_high(2);poll(1000);poll(50);poll(200);
    assert_off();assert(g_a5_completed==1); /* uint32 time wrap */
    puts("PASS: A5 defaults, all phases, finite packets, STOP priority, bootstrap config, late polling, invalid inputs, tick wrap");
    return 0;
}
