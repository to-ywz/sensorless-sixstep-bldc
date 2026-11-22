#ifndef CURRENT_FAKE_LKS_H
#define CURRENT_FAKE_LKS_H
#include <stdint.h>

typedef struct { uint32_t TRIG, IF, CFG; } ADC_TypeDef;
typedef struct { int unused; } ADC_InitTypeDef;
extern ADC_TypeDef mock_adc;
extern uint32_t MCPWM0_PRT, MCPWM0_TMR0, MCPWM0_UPDATE, MCPWM0_IF0, MCPWM0_CNT0;
#define ADCx_CFG_FSM_RS_BIT (1u << 11)
#define MCPWM0_IF0_T0_IF_BIT (1u << 0)
#define MCPWM0_IF0_TMR0_IF_BIT (1u << 10)
#define ADC0 (&mock_adc)
#define MCPWM0_UPDATE_TMR0_UPDATE_BIT (1u << 8)
#define ADC_SF1_IF (1u << 0)
#define ADC_HERR_IF (1u << 6)
#define ADC_SERR_IF (1u << 5)
#define ADC_TRIG_MCPWM0_T0 (1u << 0)
enum { ADC_CHANNEL_0, ADC_CHANNEL_1, ADC_CHANNEL_2 };
enum { CHN0 = 0, DAT0 = 0, ADC_GAIN3V6 = 0 };

void ADC_Init(ADC_TypeDef *adc, ADC_InitTypeDef *config);
void ADC_StructInit(ADC_InitTypeDef *config);
void ADC_ClearIRQFlag(ADC_TypeDef *adc, uint16_t flags);
void ADC_CHN_GAIN_CFG(ADC_TypeDef *adc, int slot, uint8_t channel, int gain);
int16_t ADC_GetConversionValue(ADC_TypeDef *adc, int slot);
#endif
