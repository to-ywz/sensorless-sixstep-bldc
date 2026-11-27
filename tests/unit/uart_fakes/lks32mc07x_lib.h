#ifndef UART_FAKE_SDK
#define UART_FAKE_SDK
#include <stdint.h>
typedef struct {
    uint32_t BUFF, IE, CTRL, IOC, RE, DIVL, DIVH, IF;
} MockUart;
extern MockUart mock_uart;
#define UART0 (&mock_uart)
#define UART0_IRQn 0
#define SYS_Module_UART0 0
#define SYS_CLK_DIV2 0u
typedef struct { uint32_t GPIO_Pin, GPIO_Mode, GPIO_PuPd; } GPIO_InitTypeDef;
#define GPIO_PuPd_UP 1u
#define GPIO_PuPd_NOPULL 0u
#define GPIO_SetBits(port, pins) ((void)(port), (void)(pins))
#define GPIO_Mode_IN 0u
#define GPIO_Mode_OUT 1u
#define GPIO_StructInit(config) ((void)(config))
#define GPIO_Init(port, config) ((void)(port), (void)(config))
#define GPIO0 0
#define GPIO1 1
#define GPIO_PinSource_0 0
#define GPIO_PinSource_15 15
#define AF4_UART 4
#define ENABLE 1
#define __NVIC_PRIO_BITS 2
#define UART_IF_SendOver 1u
#define UART_IF_RcvOver 2u
#define UART_IF_StopError 8u
#define UART_IF_CheckError 16u
#define UART_IF_RX_OV 64u
#define NVIC_DisableIRQ(irq) ((void)(irq))
#define NVIC_EnableIRQ(irq) ((void)(irq))
#define NVIC_ClearPendingIRQ(irq) ((void)(irq))
#define NVIC_SetPriority(irq, priority) ((void)(irq), (void)(priority))
#define GPIO_PinAFConfig(port, pin, af) ((void)(port), (void)(pin), (void)(af))
#define SYS_ModuleClockCmd(module, enable) ((void)(module), (void)(enable))
#define SYS_ReadMcuClk() 48000000u
#define __DMB() ((void)0)
#endif
