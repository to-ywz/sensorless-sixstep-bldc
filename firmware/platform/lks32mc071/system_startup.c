/**
 * @file system_startup.c
 * @brief CMSIS-compatible SystemInit hook used by Linko startup assembly.
 *
 * The SDK startup_lks32mc07x.s calls SystemInit before C runtime init.
 * Avoid calling SYS_MclkChoice() before .data/.bss initialization.
 * The board selects its 48 MHz test clock later in board_init_safe().
 */
void SystemInit(void)
{
    /* Deliberately keep MCU reset clock state until main(). */
}
