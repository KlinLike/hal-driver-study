/**
 * @file app_key_count.h
 * @brief 按键计数模式：消抖完成后计数与 OLED 更新
 */
#ifndef APP_KEY_COUNT_H
#define APP_KEY_COUNT_H

/** 消抖定时器到期：仅 KEY_COUNT 模式且判定为按下时计数并刷新 OLED/LED */
void app_key_count_on_debounce_done(void);
/** 整屏：标题 + 当前计数值 */
void app_key_count_ui_full(void);

#endif /* APP_KEY_COUNT_H */
