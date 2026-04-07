/**
 * @file app_config.h
 * @brief 应用层编译期配置（模式、OLED 布局、按键消抖等）
 */
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>

/** 编译期单选：时钟 / 按键计数 / 串口 IRQ / 串口 DMA+IDLE（见 APP_MODE_SELECT） */
typedef enum {
    APP_MODE_CLOCK = 0,   /**< OLED 秒表时钟 + PC13 秒闪 */
    APP_MODE_KEY_COUNT,   /**< 按键消抖后计数，OLED 显示次数 */
    APP_MODE_UART_IRQ,    /**< OLED 提示串口 IRQ 回显；按键行为同 CLOCK */
    APP_MODE_UART_DMA     /**< DMA+IDLE 不定长接收回显；按键行为同 CLOCK */
} app_mode_t;

/** 消抖时间(ms)，可在 10~50 间微调 */
#define KEY_DEBOUNCE_MS  10u

/** 运行模式：改后重新编译烧录 */
#define APP_MODE_SELECT  APP_MODE_UART_IRQ

/** OLED：正文起始列 0~15（0 起铺满宽度）；y 为页，字符占两行；偶数 y 便于对齐黄/蓝区 */
#define OLED_X_TEXT      0u
#define OLED_Y_TITLE     0u
#define OLED_Y_BODY      2u
/** UART 模式：显示最近一次 RES-> 回复的行（在 TITLE/BODY 之下） */
#define OLED_Y_UART_REPLY 4u

#endif /* APP_CONFIG_H */
