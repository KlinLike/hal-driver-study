/**
 * @file app_ui.c
 * @brief 按模式刷新 OLED 实现（接口说明见 app_ui.h）
 */
#include "app_ui.h"

#include "app_clock.h"
#include "app_config.h"
#include "app_key_count.h"
#include "app_uart_dma.h"
#include "app_uart_echo.h"
#include "driver_oled.h"

/** 清屏并按 APP_MODE_SELECT 调用对应模式的整屏绘制 */
void app_ui_full_redraw(void)
{
    OLED_Clear();
    switch (APP_MODE_SELECT) {
    case APP_MODE_CLOCK:
        app_clock_ui_full();
        break;
    case APP_MODE_KEY_COUNT:
        app_key_count_ui_full();
        break;
    case APP_MODE_UART_IRQ:
        app_uart_echo_ui_full();
        break;
    case APP_MODE_UART_DMA:
        app_uart_dma_ui_full();
        break;
    default:
        break;
    }
}
