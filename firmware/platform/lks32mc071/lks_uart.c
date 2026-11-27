#ifdef BENCH_DEBUG_UART
#include "lks_uart.h"
#include "lks32mc07x_lib.h"
#include "../../board/lks_evb_mvpowpre/board_uart.h"

enum { TX_SIZE = 256, RX_SIZE = 64 };
#define RX_ERRORS (UART_IF_StopError | UART_IF_CheckError | UART_IF_RX_OV)
#define RX_IRQS (UART_IF_RcvOver | RX_ERRORS)

/* IRQ consumes TX and produces RX. Foreground publishes TX only after copying.
 * Foreground masks this IRQ for bounded index/stat transactions and kick.
 */
static uint8_t tx[TX_SIZE], rx[RX_SIZE];
static volatile uint8_t tx_read, tx_write, rx_read, rx_write;
static volatile bool transmitting, rx_gap;
static volatile LksUartStats stats;

static void tx_next(void)
{
    transmitting = tx_read != tx_write;
    if (transmitting) {
        BOARD_UART->BUFF = tx[tx_read++];
    }
}

void lks_uart_init(void)
{
    GPIO_InitTypeDef gpio;
    uint32_t clock = SYS_ReadMcuClk() / (SYS_CLK_DIV2 + 1u);
    uint32_t divisor = (clock + BOARD_UART_BAUD / 2u) / BOARD_UART_BAUD - 1u;
    NVIC_DisableIRQ(BOARD_UART_IRQ);
    SYS_ModuleClockCmd(BOARD_UART_CLOCK_GATE, ENABLE);
    BOARD_UART->IE = 0;
    tx_read = tx_write = rx_read = rx_write = 0;
    rx_gap = transmitting = false;
    stats = (LksUartStats){0};
    BOARD_UART->CTRL = 0;
    BOARD_UART->IOC = 0;
    BOARD_UART->RE = 0;
    BOARD_UART->DIVL = divisor & 0xffu;
    BOARD_UART->DIVH = divisor >> 8;
    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = 1u << BOARD_UART_RX_SOURCE;
    gpio.GPIO_Mode = GPIO_Mode_IN;
    gpio.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(BOARD_UART_RX_PORT, &gpio);
    gpio.GPIO_Pin = 1u << BOARD_UART_TX_SOURCE;
    gpio.GPIO_Mode = GPIO_Mode_OUT;
    gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_SetBits(BOARD_UART_TX_PORT, gpio.GPIO_Pin);
    GPIO_Init(BOARD_UART_TX_PORT, &gpio);
    GPIO_PinAFConfig(BOARD_UART_RX_PORT, BOARD_UART_RX_SOURCE, BOARD_UART_AF);
    GPIO_PinAFConfig(BOARD_UART_TX_PORT, BOARD_UART_TX_SOURCE, BOARD_UART_AF);
    BOARD_UART->IF = 0x1ffu;
    BOARD_UART->IE = RX_IRQS | UART_IF_SendOver;
    NVIC_ClearPendingIRQ(BOARD_UART_IRQ);
    NVIC_SetPriority(BOARD_UART_IRQ, (1u << __NVIC_PRIO_BITS) - 1u);
    NVIC_EnableIRQ(BOARD_UART_IRQ);
}

bool lks_uart_send(const void *data, size_t length)
{
    const uint8_t *bytes = data;
    uint8_t write = tx_write;
    size_t i;
    if (length > (uint8_t)(tx_read - write - 1u)) {
        ++stats.tx_dropped;
        return false;
    }
    for (i = 0; i < length; ++i) {
        tx[write++] = bytes[i];
    }
    NVIC_DisableIRQ(BOARD_UART_IRQ);
    __DMB();
    tx_write = write;
    if (!transmitting && tx_read != tx_write) {
        BOARD_UART->IF = UART_IF_SendOver;
        tx_next();
    }
    NVIC_EnableIRQ(BOARD_UART_IRQ);
    return true;
}

bool lks_uart_receive(uint8_t *byte, bool *discontinuity)
{
    bool available;
    NVIC_DisableIRQ(BOARD_UART_IRQ);
    *discontinuity = rx_gap;
    if (rx_gap) {
        rx_read = rx_write;
        rx_gap = false;
    }
    available = rx_read != rx_write;
    if (available) {
        *byte = rx[rx_read];
        rx_read = (rx_read + 1u) & (RX_SIZE - 1u);
    }
    NVIC_EnableIRQ(BOARD_UART_IRQ);
    return available;
}

bool lks_uart_idle(void)
{
    return !transmitting; /* Cleared only after final send-complete IRQ. */
}

LksUartStats lks_uart_stats(void)
{
    LksUartStats snapshot;
    NVIC_DisableIRQ(BOARD_UART_IRQ);
    snapshot = stats;
    NVIC_EnableIRQ(BOARD_UART_IRQ);
    return snapshot;
}

void lks_uart_irq(void)
{
    uint32_t flags = BOARD_UART->IF & BOARD_UART->IE;
    if (flags & RX_IRQS) {
        uint8_t byte = (uint8_t)BOARD_UART->BUFF;
        uint8_t next = (rx_write + 1u) & (RX_SIZE - 1u);
        if (flags & RX_ERRORS) {
            ++stats.rx_errors;
            rx_gap = true;
        } else if (next == rx_read) {
            ++stats.rx_overflow;
            rx_gap = true;
        } else {
            rx[rx_write] = byte;
            rx_write = next;
        }
    }
    BOARD_UART->IF = flags;
    if (flags & UART_IF_SendOver) {
        tx_next();
    }
}
#endif
