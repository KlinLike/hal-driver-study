/**
 * @file app_uart_echo.c
 * @brief USART1 中断+IDLE 帧接收回显（接口说明见 app_uart_echo.h）
 *
 * RXNE 中断逐字节写入接收缓冲区，IDLE 中断标记一帧完成并将数据拷贝至帧缓冲区。
 * 主循环检测到帧就绪后回发 "RX[len]: <内容>\r\n"，同时刷新 OLED。
 */
#include "app_uart_echo.h"

#include "app_config.h"
#include "driver_oled.h"
#include "main.h"
#include "usart.h"

#include <string.h>
#include <stdio.h>

#define RX_BUF_SIZE  128u

/* 中断写入的接收缓冲区 */
static uint8_t   s_rx_buf[RX_BUF_SIZE];
static uint16_t  s_rx_idx;

/* 一帧完成后拷贝至此，供主循环读取 */
static uint8_t           s_frame_buf[RX_BUF_SIZE];
static volatile uint16_t s_frame_len;
static volatile uint8_t  s_frame_ready;

static void send_echo(const uint8_t *data, uint16_t len)
{
    char hdr[16];
    int  n = snprintf(hdr, sizeof(hdr), "RX[%u]: ", (unsigned)len);
    HAL_UART_Transmit(&huart1, (uint8_t *)hdr, (uint16_t)n, 50u);
    HAL_UART_Transmit(&huart1, data, len, 100u);
    HAL_UART_Transmit(&huart1, (uint8_t *)"\r\n", 2u, 10u);
}

static void update_oled(const uint8_t *data, uint16_t len)
{
    char line[17];
    uint16_t show = (len > 16u) ? 16u : len;

    for (uint16_t i = 0; i < show; i++) {
        line[i] = (data[i] >= 32u && data[i] < 127u) ? (char)data[i] : '.';
    }
    line[show] = '\0';

    /* TODO: OLED 动态刷新在 IRQ+IDLE 模式下不生效（DMA 模式正常），原因未定位 */
    OLED_ClearLine(OLED_X_TEXT, OLED_Y_UART_REPLY);
    OLED_PrintString(OLED_X_TEXT, OLED_Y_UART_REPLY, line);
}

/* ---------- 公开接口 ---------- */

void app_uart_echo_init(void)
{
    s_rx_idx      = 0u;
    s_frame_len   = 0u;
    s_frame_ready = 0u;

    __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
}

/**
 * USART1 中断里调用。
 * - RXNE：读 DR 取字节，写入接收缓冲区
 * - IDLE：读 DR 清标志（F1 系列必须），将完整帧拷贝至帧缓冲区并置就绪标志
 * - ORE ：清除溢出标志，丢弃当前不完整帧
 */
void app_uart_echo_irq_handler(void)
{
    uint32_t sr = READ_REG(huart1.Instance->SR);

    if (sr & USART_SR_RXNE) {
        uint8_t b = (uint8_t)(READ_REG(huart1.Instance->DR) & 0xFFU);
        if (s_rx_idx < RX_BUF_SIZE) {
            s_rx_buf[s_rx_idx++] = b;
        }
    }

    if (sr & USART_SR_IDLE) {
        /* F1 系列：先读 SR（已读）再读 DR 才能清除 IDLE 标志 */
        (void)READ_REG(huart1.Instance->DR);

        if (s_rx_idx > 0u) {
            if (!s_frame_ready) {
                memcpy(s_frame_buf, s_rx_buf, s_rx_idx);
                s_frame_len   = s_rx_idx;
                s_frame_ready = 1u;
            }
            s_rx_idx = 0u;
        }
    }

    if (sr & USART_SR_ORE) {
        __HAL_UART_CLEAR_OREFLAG(&huart1);
        s_rx_idx = 0u;
    }
}

void app_uart_echo_process(void)
{
    if (!s_frame_ready) return;
    s_frame_ready = 0u;

    uint16_t len = s_frame_len;
    update_oled(s_frame_buf, len);
    send_echo(s_frame_buf, len);
}

void app_uart_echo_ui_full(void)
{
    OLED_PrintString(OLED_X_TEXT, OLED_Y_TITLE, "UART IRQ+IDLE");
    OLED_PrintString(OLED_X_TEXT, OLED_Y_BODY, "Waiting RX...");
    OLED_ClearLine(OLED_X_TEXT, OLED_Y_UART_REPLY);
}
