/**
 * @file app_uart_dma.c
 * @brief USART1 DMA+IDLE 双缓冲接收 + 回显（接口说明见 app_uart_dma.h）
 *
 * 收到一帧后在主循环回发 "RX[len]: <可打印内容>\r\n"，同时刷新 OLED。
 * 参考：HAL_UARTEx_ReceiveToIdle_DMA + HAL_UARTEx_RxEventCallback
 */
#include "app_uart_dma.h"

#include "app_config.h"
#include "driver_oled.h"
#include "usart.h"

#include <string.h>
#include <stdio.h>

#define RX_BUF_SIZE  128u

static uint8_t  s_buf_a[RX_BUF_SIZE];
static uint8_t  s_buf_b[RX_BUF_SIZE];
/* DMA 当前写入缓冲区（A/B 交替） */
static uint8_t *s_active_buf  = s_buf_a;

/* 最近一次收完的一帧（由中断回调填充，主循环读取） */
static uint8_t          *s_done_buf;
static volatile uint16_t s_done_len;
static volatile uint8_t  s_rx_ready;

static void start_dma_receive(void)
{
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, s_active_buf, RX_BUF_SIZE);
    /* 关闭半传输中断，只保留 IDLE 和传输完成两种退出路径 */
    __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);
}

/* ---------- HAL 回调（中断上下文） ---------- */

/**
 * IDLE 触发或 DMA 传满时 HAL 调用此回调。
 * 策略：保存当前缓冲区指针和长度 → 切换到另一个缓冲区 → 立即重启接收 → 置标志。
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance != USART1) return;

    s_done_buf = s_active_buf;
    s_done_len = Size;

    s_active_buf = (s_active_buf == s_buf_a) ? s_buf_b : s_buf_a;
    start_dma_receive();

    s_rx_ready = 1u;
}

/** 接收出错时重启 DMA，避免卡死 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART1) return;
    start_dma_receive();
}

/* ---------- 主循环处理 ---------- */

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
    char line[17]; /* SSD1306 每行最多 16 个半角字符 + '\0' */
    uint16_t show = (len > 16u) ? 16u : len;

    for (uint16_t i = 0; i < show; i++) {
        line[i] = (data[i] >= 32u && data[i] < 127u) ? (char)data[i] : '.';
    }
    line[show] = '\0';

    OLED_ClearLine(OLED_X_TEXT, OLED_Y_UART_REPLY);
    OLED_PrintString(OLED_X_TEXT, OLED_Y_UART_REPLY, line);
}

/* ---------- 公开接口 ---------- */

void app_uart_dma_init(void)
{
    start_dma_receive();
}

void app_uart_dma_process(void)
{
    if (!s_rx_ready) return;
    s_rx_ready = 0u;

    uint8_t  *buf = s_done_buf;
    uint16_t  len = s_done_len;

    send_echo(buf, len);
    update_oled(buf, len);
}

void app_uart_dma_ui_full(void)
{
    OLED_PrintString(OLED_X_TEXT, OLED_Y_TITLE, "UART DMA+IDLE");
    OLED_PrintString(OLED_X_TEXT, OLED_Y_BODY, "Waiting RX...");
    OLED_ClearLine(OLED_X_TEXT, OLED_Y_UART_REPLY);
}
