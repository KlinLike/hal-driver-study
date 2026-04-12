/**
 * @file app_uart.c
 * @brief USART1 统一分发层——项目中唯一根据 APP_MODE_SELECT 选择 UART 实现的文件
 */
#include "app_uart.h"
#include "app_config.h"
#include <stdio.h>

#if (APP_MODE_SELECT == APP_MODE_UART_DMA)
#include "app_uart_dma.h"
#else
#include "app_uart_echo.h"
#endif

void app_uart_init(void)
{
#if (APP_MODE_SELECT == APP_MODE_UART_DMA)
    app_uart_dma_init();
#else
    app_uart_echo_init();
#endif
}

void app_uart_process(void)
{
#if (APP_MODE_SELECT == APP_MODE_UART_DMA)
    app_uart_dma_process();
#else
    app_uart_echo_process();
#endif
}

void app_uart_irq(void)
{
#if (APP_MODE_SELECT != APP_MODE_UART_DMA)
    app_uart_echo_irq_handler();
#endif
}

int app_uart_use_hal_irq(void)
{
#if (APP_MODE_SELECT == APP_MODE_UART_DMA)
    return 1;
#else
    return 0;
#endif
}

void app_uart_ui_full(void)
{
#if (APP_MODE_SELECT == APP_MODE_UART_DMA)
    app_uart_dma_ui_full();
#else
    app_uart_echo_ui_full();
#endif
}
