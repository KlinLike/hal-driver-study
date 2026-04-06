/**
 * @file app_uart_echo.c
 * @brief USART1 环形队列收、主循环 ASCII+1 回发（接口说明见 app_uart_echo.h）
 */
#include "app_uart_echo.h"

#include "app_config.h"
#include "circle_buffer.h"
#include "driver_oled.h"
#include "main.h"
#include "usart.h"

#define UART_RX_RING_SIZE  256u

static circle_buffer_t s_rx_ring;
static uint8_t s_rx_pool[UART_RX_RING_SIZE];

/* 将 (rx+1) 格式化为 RES-> …；串口发 CRLF；UART 模式下同步刷新 OLED 下一行 */
static void uart_send_res_plus1(uint8_t rx_byte)
{
    uint8_t out = (uint8_t)(rx_byte + 1u);
    char buf[24];
    uint32_t n = 0;

    buf[n++] = 'R';
    buf[n++] = 'E';
    buf[n++] = 'S';
    buf[n++] = '-';
    buf[n++] = '>';
    buf[n++] = ' ';

    if (out >= 32u && out < 127u) {
        buf[n++] = (char)out;
    } else {
        static const char hex[] = "0123456789ABCDEF";
        buf[n++] = '0';
        buf[n++] = 'x';
        buf[n++] = hex[(out >> 4) & 0x0Fu];
        buf[n++] = hex[out & 0x0Fu];
    }

    if (APP_MODE_SELECT == APP_MODE_UART_IRQ) {
        buf[n] = '\0';
        OLED_ClearLine(OLED_X_TEXT, OLED_Y_UART_REPLY);
        OLED_PrintString(OLED_X_TEXT, OLED_Y_UART_REPLY, buf);
    }

    buf[n++] = '\r';
    buf[n++] = '\n';
    (void)HAL_UART_Transmit(&huart1, (uint8_t *)buf, (uint16_t)n, 100u);
}

/** 初始化接收环形队列（须在 MX_USART1_UART_Init 之前调用） */
void app_uart_echo_init(void)
{
    circle_buffer_init(&s_rx_ring, s_rx_pool, UART_RX_RING_SIZE);
}

/** USART1 中断里调用：读 DR 入队、处理 ORE */
void app_uart_echo_irq_handler(void)
{
    uint32_t sr = READ_REG(huart1.Instance->SR);

    if ((sr & USART_SR_RXNE) != 0U) {
        uint8_t b = (uint8_t)(READ_REG(huart1.Instance->DR) & 0xFFU);
        if (circle_buffer_write(&s_rx_ring, b) != 0) {
            /* 队列满则丢弃 */
        }
        return;
    }

    if ((sr & USART_SR_ORE) != 0U) {
        __HAL_UART_CLEAR_OREFLAG(&huart1);
    }
}

/** 主循环：队列非空则取字节、+1、格式化发送 */
void app_uart_echo_process(void)
{
    uint8_t b;

    while (circle_buffer_read(&s_rx_ring, &b) == 0) {
        uart_send_res_plus1(b);
    }
}

/** 整屏：UART 模式下的 OLED 标题与说明行 */
void app_uart_echo_ui_full(void)
{
    OLED_PrintString(OLED_X_TEXT, OLED_Y_TITLE, "UART IRQ");
    OLED_PrintString(OLED_X_TEXT, OLED_Y_BODY, "RX +1 RES->");
    OLED_ClearLine(OLED_X_TEXT, OLED_Y_UART_REPLY);
}
