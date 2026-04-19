/**
 * @file app_key_count.c
 * @brief 按键计数实现（接口说明见 app_key_count.h）
 */
#include "app_key_count.h"

#include "app_board.h"
#include "app_config.h"
#include "driver_oled.h"
#include "main.h"

static uint32_t s_key_press_count;

/** 消抖定时器到期：仅 KEY_COUNT 模式且判定为按下时计数并刷新 OLED/LED */
void app_key_count_on_debounce_done(void)
{
    switch (APP_MODE_SELECT) {
    case APP_MODE_CLOCK:
    case APP_MODE_UART_IRQ:
        break;
    case APP_MODE_KEY_COUNT: {
        GPIO_PinState level = HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_GPIO_Pin);
        if (level == GPIO_PIN_RESET) {
            s_key_press_count++;
            OLED_ClearLine(OLED_X_TEXT, OLED_Y_BODY);
            OLED_PrintSignedVal(OLED_X_TEXT, OLED_Y_BODY, (int32_t)s_key_press_count);
            LED_Control((int)(s_key_press_count & 1u));
            BUZZ_Control(0);
        }
        break;
    }
    default:
        break;
    }
}

/** 整屏：标题 + 当前计数值 */
void app_key_count_ui_full(void)
{
    OLED_PrintString(OLED_X_TEXT, OLED_Y_TITLE, "Key Count");
    OLED_PrintSignedVal(OLED_X_TEXT, OLED_Y_BODY, (int32_t)s_key_press_count);
}
