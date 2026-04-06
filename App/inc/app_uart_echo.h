/**
 * @file app_uart_echo.h
 * @brief USART1：中断单字节入环形队列，主循环回发 ASCII+1（RES-> …）
 */
#ifndef APP_UART_ECHO_H
#define APP_UART_ECHO_H

#include <stdint.h>

/** 初始化接收环形队列（须在 MX_USART1_UART_Init 之前调用） */
void app_uart_echo_init(void);
/** USART1 中断里调用：读 DR 入队、处理 ORE */
void app_uart_echo_irq_handler(void);
/** 主循环：队列非空则取字节、+1、格式化发送 */
void app_uart_echo_process(void);

#endif /* APP_UART_ECHO_H */
