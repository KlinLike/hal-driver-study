/**
 * @file app_uart_echo.h
 * @brief USART1：RXNE 中断逐字节收帧，IDLE 中断判断帧边界，主循环整帧回显
 */
#ifndef APP_UART_ECHO_H
#define APP_UART_ECHO_H

#include <stdint.h>

/** 初始化帧缓冲区（须在 MX_USART1_UART_Init 之后调用） */
void app_uart_echo_init(void);
/** USART1 中断里调用：处理 RXNE / IDLE / ORE */
void app_uart_echo_irq_handler(void);
/** 主循环：帧就绪则整帧回显并刷新 OLED */
void app_uart_echo_process(void);
/** 整屏：UART 模式下的 OLED 标题与说明行 */
void app_uart_echo_ui_full(void);

#endif /* APP_UART_ECHO_H */
