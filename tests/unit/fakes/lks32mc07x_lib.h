/* Host-only stub; never put tests/unit/fakes in Keil include paths. */
#ifndef A4_FAKE_LIB_H
#define A4_FAKE_LIB_H
#include <stdint.h>
#define GPIO2 2u
#define GPIO_Pin_11 (1u<<11)
#define GPIO_Pin_12 (1u<<12)
#define GPIO_PinSource_11 11u
#define GPIO_PinSource_12 12u
#define AF0_GPIO 0u
#define GPIO_Mode_IN 0u
#define GPIO_PuPd_NOPULL 0u
typedef struct {uint32_t GPIO_Pin,GPIO_Mode,GPIO_PuPd;} GPIO_InitTypeDef;
void GPIO_PinAFConfig(uint32_t,uint32_t,uint32_t);
void GPIO_StructInit(GPIO_InitTypeDef*);
void GPIO_Init(uint32_t,GPIO_InitTypeDef*);
uint32_t GPIO_ReadInputDataBit(uint32_t,uint32_t);
#endif
