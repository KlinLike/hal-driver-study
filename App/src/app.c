/**
 * @file app.c
 * @brief 应用入口实现（接口说明见 app.h）
 */
#include "app.h"

#include <stddef.h>

#include "app_board.h"
#include "app_clock.h"
#include "app_config.h"
#include "app_key_count.h"
#include "app_mpu6050.h"
#include "app_uart.h"
#include "app_ui.h"
#include "driver_oled.h"
#include "soft_timer.h"

static soft_timer_t s_clock_timer;
static soft_timer_t s_key_timer;

/* 1s 周期回调：仅时钟模式累加秒、翻 PC13 */
static void clock_timer_cb(void *arg)
{
    (void)arg;
    app_clock_on_timer_tick();
}

/* 消抖到期回调：按模式处理按键（计数等） */
static void key_timer_cb(void *arg)
{
    (void)arg;
    app_key_count_on_debounce_done();
}

/** 应用初始化：关声光、OLED、软定时器、按模式刷界面（USART 在 main 已 init） */
void app_init(void)
{
    /* USART1 与 DMA/环形队列已在 main 中初始化 */

    LED_Control(0);
    BUZZ_Control(0);
    OLED_Init();

    soft_timer_init(&s_clock_timer, 1000u, SOFT_TIMER_MODE_PERIODIC, clock_timer_cb, NULL);
    soft_timer_start(&s_clock_timer);

    soft_timer_init(&s_key_timer, KEY_DEBOUNCE_MS, SOFT_TIMER_MODE_ONESHOT, key_timer_cb, NULL);

#if (APP_MODE_SELECT == APP_MODE_MPU6050_POLL)
    app_mpu6050_init();
#endif

    app_ui_full_redraw();
}

/** SysTick 1ms 里调用，驱动软定时器（时钟秒、按键消抖） */
void app_systick_handler(void)
{
    soft_timer_process(&s_clock_timer);
    soft_timer_process(&s_key_timer);
}

/** 主循环调用：处理串口队列、时钟 OLED 刷新标志 */
void app_poll(void)
{
    app_uart_process();
    app_clock_poll();
    /* MPU6050 模式：所有 I2C 操作在 app_mpu6050_init() 一次性完成，主循环无需轮询 */
}

/** 按键 EXTI 触发，启动消抖单次定时器 */
void app_on_key_exti(void)
{
    soft_timer_start(&s_key_timer);
}
