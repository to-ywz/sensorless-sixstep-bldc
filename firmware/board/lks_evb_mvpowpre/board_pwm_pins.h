#ifndef BOARD_PWM_PINS_H
#define BOARD_PWM_PINS_H
/* LKS_EVB_MCU071CBT8_V1.0 -> MVPOWPRE_V5.0 LKS560.
 * Datasheet 1.3 / 2.2: HIN active high, LIN active LOW.
 * P/N are physical MCU pads, NOT both active-high gate requests.
 */
#define BOARD_PWM_FIRST_PIN 4u
#define BOARD_PWM_LAST_PIN  9u
#define BOARD_PWM_HIGH_SIDE_PINS (GPIO_Pin_4 | GPIO_Pin_6 | GPIO_Pin_8)
#define BOARD_PWM_LOW_SIDE_PINS  (GPIO_Pin_5 | GPIO_Pin_7 | GPIO_Pin_9)
#define BOARD_PWM_PIN_MASK (BOARD_PWM_HIGH_SIDE_PINS | BOARD_PWM_LOW_SIDE_PINS)
#define BOARD_DRIVER_HIN_OFF 0u
#define BOARD_DRIVER_LIN_OFF 1u
#define BOARD_DRIVER_LIN_ON  0u
#endif
