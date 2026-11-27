#ifndef LKS_UART_H
#define LKS_UART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t tx_dropped;
    uint32_t rx_overflow;
    uint32_t rx_errors;
} LksUartStats;

void lks_uart_init(void);
bool lks_uart_send(const void *data, size_t length);
bool lks_uart_receive(uint8_t *byte, bool *discontinuity);
bool lks_uart_idle(void);
LksUartStats lks_uart_stats(void);
void lks_uart_irq(void);

#endif
