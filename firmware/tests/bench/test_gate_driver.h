#ifndef TEST_GATE_DRIVER_H
#define TEST_GATE_DRIVER_H
#include <stdint.h>
void bench_gate_driver_init(void);
void bench_gate_driver_poll(void);
/* Watch: phase 0=U,1=V,2=W; test 0=low hold,1=precharge+high PWM packet.
 * command 1=RUN (or START edge),2=STOP; consumed once, never auto-repeat.
 * bootstrap_checked: bit0/1/2 set ONLY after measuring that phase VB-VS.
 * This flag is operator configuration, NOT a hardware voltage interlock.
 */
extern volatile uint32_t g_a5_phase, g_a5_test, g_a5_command;
extern volatile uint32_t g_a5_bootstrap_checked;
extern volatile uint32_t g_a5_state, g_a5_fault, g_a5_completed;
/* States:0=OFF,1=LOW_HOLD,2=PRECHARGE,3=ALL_OFF_GAP,4=HIGH_PACKET.
 * Faults:0=none,1=bad input,2=bootstrap not checked,3=output rejected,
 * 4=configuration changed while active,5=excessive precharge/gap lateness.
 */
#endif
