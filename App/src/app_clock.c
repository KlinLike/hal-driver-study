/**
 * @file app_clock.c
 * @brief OLED 时钟实现（接口说明见 app_clock.h）
 */
#include "app_clock.h"

#include "app_config.h"
#include "driver_oled.h"
#include "main.h"

static volatile uint32_t s_clock_seconds;
static volatile uint8_t s_clock_update_flag;

/** 软定时器 1s 回调：秒++、置刷新标志、闪 LED */
void app_clock_on_timer_tick(void)
{
    s_clock_seconds++;
    s_clock_update_flag = 1;
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_GPIO_Pin);
}

/** 在正文区画一行 HH:MM:SS */
void app_clock_draw_line(void)
{
    uint32_t s = s_clock_seconds;
    uint32_t hh = (s / 3600u) % 24u;
    uint32_t mm = (s / 60u) % 60u;
    uint32_t ss = s % 60u;
    char buf[9];

    buf[0] = (char)('0' + hh / 10u);
    buf[1] = (char)('0' + hh % 10u);
    buf[2] = (ss & 1u) ? ':' : ' ';
    buf[3] = (char)('0' + mm / 10u);
    buf[4] = (char)('0' + mm % 10u);
    buf[5] = buf[2];
    buf[6] = (char)('0' + ss / 10u);
    buf[7] = (char)('0' + ss % 10u);
    buf[8] = '\0';
    OLED_PrintString(OLED_X_TEXT, OLED_Y_BODY, buf);
}

/** 主循环：有刷新标志时重画时间行 */
void app_clock_poll(void)
{
    if (s_clock_update_flag == 0u) {
        return;
    }
    s_clock_update_flag = 0;
    app_clock_draw_line();
}

/** 整屏：标题 + 当前时间行 */
void app_clock_ui_full(void)
{
    OLED_PrintString(OLED_X_TEXT, OLED_Y_TITLE, "LED Clock");
    app_clock_draw_line();
}
