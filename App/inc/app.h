/**
 * @file app.h
 * @brief 应用入口：初始化、1ms 软定时器轮询、主循环轮询、按键 EXTI 触发消抖
 */
#ifndef APP_H
#define APP_H

/** 应用初始化：关声光、OLED、软定时器、按模式刷界面（USART 在 main 已 init） */
void app_init(void);
/** SysTick 1ms 里调用，驱动软定时器（时钟秒、按键消抖） */
void app_systick_handler(void);
/** 主循环调用：处理串口队列、时钟 OLED 刷新标志 */
void app_poll(void);
/** 按键 EXTI 触发，启动消抖单次定时器 */
void app_on_key_exti(void);

#endif /* APP_H */
