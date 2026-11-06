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
 * - Power driver HIN/LIN active level has not yet been validated.
 *
 * The six PWM-capable pads remain GPIO outputs driven low.
 * This function MUST NOT enable MCPWM or MOSFET gate drivers.
 */
void board_init_safe(void);

/** @brief Toggle the logic-level heartbeat output P0.6. */
void board_heartbeat_toggle(void);

#endif /* BOARD_INIT_H */
