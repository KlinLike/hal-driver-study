/**
 * @file app_clock.h
 * @brief OLED 时钟：秒计数、1Hz LED、时间行刷新
 */
#ifndef APP_CLOCK_H
#define APP_CLOCK_H

/** 软定时器 1s 回调：仅 APP_MODE_CLOCK 时秒++、置刷新标志、闪 LED */
void app_clock_on_timer_tick(void);
/** 主循环：有刷新标志时重画时间行 */
void app_clock_poll(void);
/** 在正文区画一行 HH:MM:SS */
void app_clock_draw_line(void);
/** 整屏：标题 + 当前时间行 */
void app_clock_ui_full(void);

#endif /* APP_CLOCK_H */
