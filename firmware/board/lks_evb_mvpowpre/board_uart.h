#ifndef BOARD_UART_H
#define BOARD_UART_H

/* MCU071CBT8 V1.0 + MVPOWPRE V5.0 P4: GND, TXD0, RXD0. */
#define BOARD_UART UART0
#define BOARD_UART_CLOCK_GATE SYS_Module_UART0
#define BOARD_UART_IRQ UART0_IRQn
#define BOARD_UART_TX_PORT GPIO1
#define BOARD_UART_TX_SOURCE GPIO_PinSource_0
#define BOARD_UART_RX_PORT GPIO0
#define BOARD_UART_RX_SOURCE GPIO_PinSource_15
#define BOARD_UART_AF AF4_UART
/* 3 Mbps exceeds the range listed in the LKS32MC07x datasheet.
 * Enable explicitly for isolated bench characterization.
 */
#ifdef BENCH_DEBUG_UART_3M_EXPERIMENTAL
#define BOARD_UART_BAUD 3000000u
#else
#define BOARD_UART_BAUD 115200u
#endif

#endif
