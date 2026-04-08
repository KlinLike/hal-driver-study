/**
 * @file app_uart_dma.h
 * @brief USART1 DMA+IDLE 不定长接收回显（双缓冲）
 *
 * 工作原理：
 *   DMA 自动搬运 RX 数据到缓冲区，IDLE 空闲中断通知"一帧结束"，
 *   回调中立即切换缓冲区并重启 DMA，主循环处理上一帧数据。
 */
#ifndef APP_UART_DMA_H
#define APP_UART_DMA_H

#include <stdint.h>

/** 启动首次 DMA+IDLE 接收（须在 MX_USART1_UART_Init 之后调用） */
void app_uart_dma_init(void);
/** 主循环：检测帧就绪标志，格式化回发 + OLED 显示 */
void app_uart_dma_process(void);
/** 整屏：DMA 模式下的 OLED 标题与说明行 */
void app_uart_dma_ui_full(void);

#endif /* APP_UART_DMA_H */
