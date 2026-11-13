/* Host register model, not vendor code. Never include in a Keil target.
 * Modeled fields are limited to what production sources under test use.
 * GPIO, CH_DEF, CH_MASK and per-channel physical polarity are independent
 * of the assertions for intended LKS560 output behavior.
 */
#ifndef LKS560_TEST_LIB_H
#define LKS560_TEST_LIB_H
#include <stdint.h>
typedef uint32_t FuncState;
#define ENABLE 1u
#define DISABLE 0u
#define MCPWM0_LOW_LEVEL 0u
#define MCPWM0_HIGH_LEVEL 1u
#define MCPWM0_CENTRAL_PWM_MODE 0u
#define GPIO0 0u
#define GPIO1 1u
#define GPIO2 2u
#define AF0_GPIO 0u
#define AF3_MCPWM 3u
#define GPIO_Mode_IN 0u
#define GPIO_Mode_OUT 1u
#define GPIO_PuPd_NOPULL 0u
#define SYS_Module_GPIO 1u
#define SYS_MCLK_48M_RC 48u
#define __NOP() ((void)0)
#define __DSB() ((void)0)
#define __disable_irq() ((void)0)
#define __enable_irq() ((void)0)
#define IWDG_DISABLE() ((void)0)
#define BIT0 (1u<<0)
#define GPIO_Pin_0 (1u<<0)
#define GPIO_PinSource_0 0u
#define BIT1 (1u<<1)
#define GPIO_Pin_1 (1u<<1)
#define GPIO_PinSource_1 1u
#define BIT2 (1u<<2)
#define GPIO_Pin_2 (1u<<2)
#define GPIO_PinSource_2 2u
#define BIT3 (1u<<3)
#define GPIO_Pin_3 (1u<<3)
#define GPIO_PinSource_3 3u
#define BIT4 (1u<<4)
#define GPIO_Pin_4 (1u<<4)
#define GPIO_PinSource_4 4u
#define BIT5 (1u<<5)
#define GPIO_Pin_5 (1u<<5)
#define GPIO_PinSource_5 5u
#define BIT6 (1u<<6)
#define GPIO_Pin_6 (1u<<6)
#define GPIO_PinSource_6 6u
#define BIT7 (1u<<7)
#define GPIO_Pin_7 (1u<<7)
#define GPIO_PinSource_7 7u
#define BIT8 (1u<<8)
#define GPIO_Pin_8 (1u<<8)
#define GPIO_PinSource_8 8u
#define BIT9 (1u<<9)
#define GPIO_Pin_9 (1u<<9)
#define GPIO_PinSource_9 9u
#define BIT10 (1u<<10)
#define GPIO_Pin_10 (1u<<10)
#define GPIO_PinSource_10 10u
#define BIT11 (1u<<11)
#define GPIO_Pin_11 (1u<<11)
#define GPIO_PinSource_11 11u
#define BIT12 (1u<<12)
#define GPIO_Pin_12 (1u<<12)
#define GPIO_PinSource_12 12u
#define BIT13 (1u<<13)
#define GPIO_Pin_13 (1u<<13)
#define GPIO_PinSource_13 13u
#define BIT14 (1u<<14)
#define GPIO_Pin_14 (1u<<14)
#define GPIO_PinSource_14 14u
#define BIT15 (1u<<15)
#define GPIO_Pin_15 (1u<<15)
#define GPIO_PinSource_15 15u
#define MCPWM0_FAIL012_MOE_BIT BIT6
#define MCPWM0_IO01_CH0_NP_BIT BIT0
#define MCPWM0_IO01_CH0_NS_BIT BIT2
#define MCPWM0_IO01_CH0_PS_BIT BIT3
#define MCPWM0_IO01_CH0_SCTRLN_BIT BIT4
#define MCPWM0_TCLK_BASE_CNT0_EN_BIT BIT6
#define MCPWM0_UPDATE_CNT0_UPDATE_BIT BIT11
#define MCPWM0_UPDATE_TH00_UPDATE_BIT BIT0
#define MCPWM0_UPDATE_TH01_UPDATE_BIT BIT1
#define MCPWM0_UPDATE_TH10_UPDATE_BIT BIT2
#define MCPWM0_UPDATE_TH11_UPDATE_BIT BIT3
#define MCPWM0_UPDATE_TH20_UPDATE_BIT BIT4
#define MCPWM0_UPDATE_TH21_UPDATE_BIT BIT5
typedef struct {
    uint32_t AUEN;
    uint32_t BASE_CNT0_EN;
    uint32_t CH0N_FAIL_EN;
    uint32_t CH0N_Polarity_INV;
    uint32_t CH0N_SCTRLN;
    uint32_t CH0N_default_output;
    uint32_t CH0P_FAIL_EN;
    uint32_t CH0P_Polarity_INV;
    uint32_t CH0P_SCTRLP;
    uint32_t CH0P_default_output;
    uint32_t CH0_NS;
    uint32_t CH0_PS;
    uint32_t CH1N_FAIL_EN;
    uint32_t CH1N_Polarity_INV;
    uint32_t CH1N_SCTRLN;
    uint32_t CH1N_default_output;
    uint32_t CH1P_FAIL_EN;
    uint32_t CH1P_Polarity_INV;
    uint32_t CH1P_SCTRLP;
    uint32_t CH1P_default_output;
    uint32_t CH1_NS;
    uint32_t CH1_PS;
    uint32_t CH2N_FAIL_EN;
    uint32_t CH2N_Polarity_INV;
    uint32_t CH2N_SCTRLN;
    uint32_t CH2N_default_output;
    uint32_t CH2P_FAIL_EN;
    uint32_t CH2P_Polarity_INV;
    uint32_t CH2P_SCTRLP;
    uint32_t CH2P_default_output;
    uint32_t CH2_NS;
    uint32_t CH2_PS;
    uint32_t CLK_DIV;
    uint32_t DeadTimeCH012N;
    uint32_t DeadTimeCH012P;
    uint32_t IO0_PPE;
    uint32_t IO1_PPE;
    uint32_t IO2_PPE;
    uint32_t MCLK_EN;
    uint32_t MCPWM_WorkModeCH0;
    uint32_t MCPWM_WorkModeCH1;
    uint32_t MCPWM_WorkModeCH2;
    uint32_t Switch_CH0N_CH0P;
    uint32_t Switch_CH1N_CH1P;
    uint32_t Switch_CH2N_CH2P;
    uint32_t TH0;
    uint32_t TH00;
    uint32_t TH01;
    uint32_t TH10;
    uint32_t TH11;
    uint32_t TH20;
    uint32_t TH21;
    uint32_t TR0_AEC;
    uint32_t TR0_T0_UpdateEN;
} MCPWM_InitTypeDef;
typedef struct {uint32_t GPIO_Pin,GPIO_Mode,GPIO_PuPd;} GPIO_InitTypeDef;
typedef struct {uint32_t CTRL,LOAD,VAL;} FakeSysTick;
FakeSysTick *fake_systick(void);
#define SysTick (fake_systick())
#define SysTick_CTRL_CLKSOURCE_Msk 4u
#define SysTick_CTRL_ENABLE_Msk 1u
extern uint32_t MCPWM0_PRT,MCPWM0_TCLK,MCPWM0_IO01,MCPWM0_IO23;
extern uint32_t MCPWM0_TH00,MCPWM0_TH01,MCPWM0_TH10,MCPWM0_TH11,MCPWM0_TH20,MCPWM0_TH21;
extern uint32_t MCPWM0_CNT0,MCPWM0_UPDATE,MCPWM0_FAIL012;
void GPIO_PinAFConfig(uint32_t,uint32_t,uint32_t);
void GPIO_StructInit(GPIO_InitTypeDef*);
void GPIO_Init(uint32_t,GPIO_InitTypeDef*);
void GPIO_ResetBits(uint32_t,uint32_t);
void GPIO_SetBits(uint32_t,uint32_t);
uint32_t GPIO_ReadInputDataBit(uint32_t,uint32_t);
uint32_t GPIO_ReadOutputDataBit(uint32_t,uint32_t);
void SYS_ModuleClockCmd(uint32_t,uint32_t);
void SYS_MclkChoice(uint32_t);
void MCPWM_StructInit(MCPWM_InitTypeDef*);
void MCPWM_Init(MCPWM_InitTypeDef*);
void PWMOutputs(FuncState);
#endif
