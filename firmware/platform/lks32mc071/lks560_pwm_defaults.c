#include "lks560_pwm_defaults.h"
void lks560_pwm_set_off_defaults(MCPWM_InitTypeDef *cfg)
{
    cfg->CH0P_default_output = cfg->CH1P_default_output =
        cfg->CH2P_default_output = MCPWM0_LOW_LEVEL;
    cfg->CH0N_default_output = cfg->CH1N_default_output =
        cfg->CH2N_default_output = MCPWM0_HIGH_LEVEL;   // 默认三相低电平
    cfg->CH0P_FAIL_EN = cfg->CH0N_FAIL_EN = cfg->CH1P_FAIL_EN =
        cfg->CH1N_FAIL_EN = cfg->CH2P_FAIL_EN = cfg->CH2N_FAIL_EN = ENABLE;  // 开启异常急停
    cfg->TR0_AEC = DISABLE; /* 自动清除错误关闭 */
}
