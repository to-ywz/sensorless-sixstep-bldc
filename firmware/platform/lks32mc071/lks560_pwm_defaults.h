#ifndef LKS560_PWM_DEFAULTS_H
#define LKS560_PWM_DEFAULTS_H
#include "lks32mc07x_lib.h"
/* CH_DEF bypasses channel polarity inversion and swaps (MCU manual CH_DEF).
 * Therefore N defaults must be physical HIGH, even when normal N is inverted.
 * FAIL masks cover software MOE clearing as well as hardware fault events.
 * This does not configure or qualify a comparator/Fail fault source.
 */
void lks560_pwm_set_off_defaults(MCPWM_InitTypeDef *cfg);
#endif
