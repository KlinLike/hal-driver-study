/**
 * @file app_uart.h
 * @brief USART1 统一接口——编译期按 APP_MODE_SELECT 分发到 IRQ 或 DMA 实现
 *
 * 所有调用方只包含此头文件，不直接引用 app_uart_echo.h 或 app_uart_dma.h。
 */
#ifndef APP_UART_H
#define APP_UART_H

void app_uart_init(void);
void app_uart_process(void);
void app_uart_irq(void);
int  app_uart_use_hal_irq(void);
void app_uart_ui_full(void);

#endif /* APP_UART_H */
