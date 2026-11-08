#ifndef TEST_SIXSTEP_LOGIC_H
#define TEST_SIXSTEP_LOGIC_H
#include <stdint.h>
void bench_sixstep_logic_init(void);
void bench_sixstep_logic_poll(void);
/* Keil Watch controls: set configuration while stopped, then press START.
 * mode:0=manual,1=auto; reverse:0=forward,1=reverse; duty:100..500 permille.
 * command:0=none,1=start/step,2=stop,3=apply test sector (requires running).
 * command3 deliberately permits invalid sector injection to verify safe stop.
 */
extern volatile uint32_t g_a4_mode, g_a4_reverse, g_a4_duty_permille;
extern volatile uint32_t g_a4_command, g_a4_test_sector;
extern volatile uint32_t g_a4_sector, g_a4_running, g_a4_fault, g_a4_transitions;
#endif
