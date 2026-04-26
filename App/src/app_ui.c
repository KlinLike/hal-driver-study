/**
 * @file app_ui.c
 * @brief 按模式刷新 OLED 实现（接口说明见 app_ui.h）
 */
#include "app_ui.h"

#include "app_clock.h"
#include "app_config.h"
#include "app_key_count.h"
#include "app_mpu6050.h"
#include "app_uart.h"
#include "driver_oled.h"

/* 16x16 汉字点阵：大(0) 西(1) 瓜(2)
 * 每个汉字 32 字节：前 16 字节为上半部，后 16 字节为下半部。 */
static const uint8_t s_demo_chinese16x16[][32] = {
    {
        0x00, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xFC, 0xA0, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00,
        0x00, 0x80, 0x80, 0x40, 0x20, 0x18, 0x06, 0x01, 0x01, 0x06, 0x18, 0x20, 0x40, 0x80, 0x80, 0x00
    }, /* 大 */
    {
        0x00, 0xC4, 0x44, 0x44, 0x44, 0xFC, 0x44, 0x44, 0x44, 0xFC, 0x44, 0x44, 0x44, 0x44, 0xC4, 0x00,
        0x00, 0x7F, 0x90, 0x90, 0x88, 0x87, 0x80, 0x80, 0x80, 0x87, 0x88, 0x88, 0x88, 0x88, 0xFF, 0x00
    }, /* 西 */
    {
        0x00, 0x00, 0xF8, 0x04, 0x04, 0x04, 0xFC, 0x04, 0x04, 0x04, 0x04, 0x1C, 0xE4, 0x00, 0x00, 0x00,
        0x80, 0x60, 0x1F, 0x00, 0x00, 0x00, 0x7F, 0x80, 0x40, 0x46, 0x38, 0x60, 0x07, 0x38, 0x40, 0x00
    }  /* 瓜 */
};

static void app_oled_chinese_ui_full(void)
{
    uint8_t chinese_count = (uint8_t)(sizeof(s_demo_chinese16x16) / sizeof(s_demo_chinese16x16[0]));
    uint8_t total_cols = (uint8_t)(chinese_count * 16u);
    uint8_t start_col = (uint8_t)((128u - total_cols) / 2u);
    uint8_t start_page = (uint8_t)((8u - 2u) / 2u);

    OLED_PrintString(OLED_X_TEXT, OLED_Y_TITLE, "OLED Chinese");
    OLED_PrintChinese16x16(start_col, start_page, (const uint8_t *)s_demo_chinese16x16, chinese_count);
}

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
    case APP_MODE_UART_DMA:
        app_uart_ui_full();
        break;
    case APP_MODE_MPU6050_POLL:
    case APP_MODE_MPU6050_IT:
        app_mpu6050_ui_full();
        break;
    case APP_MODE_OLED_CHINESE:
        app_oled_chinese_ui_full();
        break;
    default:
        break;
    }
}
