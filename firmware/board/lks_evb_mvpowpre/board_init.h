/**
 * @file board_init.h
 * @brief Safe initialization contract for LKS_EVB_MCU071CBT8_V1.0.
 */
#ifndef BOARD_INIT_H
#define BOARD_INIT_H

/**
 * @brief Configure the board for logic-only testing.
 *
 * Preconditions:
 * - Three-phase power stage is not energized.
 * - FN550-90K is disconnected.
 * - LKS560 HIN=0/LIN=1 is the required software OFF command.
 *
 * P1.4/6/8 (HIN) are low; P1.5/7/9 (LIN) are high.
 * This does not guarantee reset/high-impedance hardware safety.
 * This function MUST NOT enable MCPWM or MOSFET gate drivers.
 */
void board_init_safe(void);

/** @brief Restore LKS560 GPIO OFF (HIN=0, LIN=1).
 * Disable active MCPWM outputs first; also used before PWM initialization. */
void board_pwm_inputs_off(void);

/** @brief Toggle the logic-level heartbeat output P0.6. */
void board_heartbeat_toggle(void);

#endif /* BOARD_INIT_H */
