#include <assert.h>
#include <stdio.h>
#include "lks32mc07x_lib.h"
#include "sixstep_table.h"
#include "bench_sixstep_output.h"
#include "test_sixstep_logic.h"
static uint32_t now, start_pressed, stop_pressed, output_sector=255u;
void GPIO_PinAFConfig(uint32_t a,uint32_t b,uint32_t c){(void)a;(void)b;(void)c;}
void GPIO_StructInit(GPIO_InitTypeDef *p){p->GPIO_Pin=p->GPIO_Mode=p->GPIO_PuPd=0;}
void GPIO_Init(uint32_t a,GPIO_InitTypeDef *p){(void)a;(void)p;}
uint32_t GPIO_ReadInputDataBit(uint32_t a,uint32_t p)
{(void)a;return !(p==GPIO_Pin_11?start_pressed:stop_pressed);}
uint32_t bench_sixstep_clock(void){return now;}
uint32_t bench_sixstep_ticks_per_ms(void){return 1000u;}
void bench_sixstep_output_stop(void){output_sector=255u;}
void bench_sixstep_output_init(void){bench_sixstep_output_stop();}
int bench_sixstep_output_apply(uint32_t s,uint32_t d)
{SixstepState state;bench_sixstep_output_stop();if(stop_pressed ||
 !sixstep_get(s,&state)||!sixstep_duty_valid(d)) { return 0; }
 output_sector=s;return 1;}
static void poll(uint32_t us){now+=us;bench_sixstep_logic_poll();}
static void command(uint32_t c){g_a4_command=c;poll(0);}
static void init(uint32_t held)
{now=0;start_pressed=held;stop_pressed=0;g_a4_mode=g_a4_reverse=0;
 g_a4_duty_permille=250;bench_sixstep_logic_init();}
static void button(uint32_t pressed)
{start_pressed=pressed;poll(0);poll(20000);}
int main(void)
{
 unsigned i;
 init(1);poll(30000);assert(!g_a4_running && output_sector==255);
 button(0);button(1);assert(g_a4_running && output_sector==0);
 poll(100000);assert(output_sector==0); /* held START never repeats */
 button(0);button(1);assert(output_sector==1);
 stop_pressed=1;command(1);assert(!g_a4_running && output_sector==255);
 stop_pressed=0;poll(30000);assert(!g_a4_running);
 button(0);button(1);assert(output_sector==0);
 g_a4_reverse=1;poll(0);assert(!g_a4_running);
 command(1);assert(output_sector==0);button(0);button(1);assert(output_sector==5);
 command(2);g_a4_mode=1;g_a4_reverse=0;command(1);
 for(i=0;i<60;i++){poll(99999);assert(output_sector==i%6);
   poll(1);assert(output_sector==(i+1)%6);}
 command(2);g_a4_reverse=1;command(1);
 for(i=0;i<60;i++){poll(100000);assert(output_sector==(5-(i%6)));}
 command(2);g_a4_mode=0;command(1);
 g_a4_test_sector=6;command(3);assert(!g_a4_running && g_a4_fault && output_sector==255);
 g_a4_duty_permille=99;command(1);assert(!g_a4_running && g_a4_fault);
 g_a4_duty_permille=250;command(1);assert(g_a4_running);
 command(99);assert(!g_a4_running && g_a4_fault);
 command(2);g_a4_mode=1;now=0xffff0000u;command(1);
 poll(100000);assert(output_sector==5); /* reverse, uint32 tick wrap */
 /* STOP/START simultaneous and a later release cannot restart by itself. */
 stop_pressed=1;start_pressed=1;command(1);
 stop_pressed=0;button(0);assert(!g_a4_running);button(1);assert(g_a4_running);
 puts("bench debounce, STOP priority, config restart, 10 forward/reverse cycles, invalid input, tick wrap: PASS");
 return 0;
}
