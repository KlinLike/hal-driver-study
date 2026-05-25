/**
 * @file app_color_led.h
 * @brief PWM 三色 LED 演示：TIM2 三通道 PWM 控制 RGB（课程 054-055 配套）
 *
 * 硬件：共阳极 RGB LED，低电平点亮。
 * 通道映射（以 CubeMX 实际配置为准）：
 *   红(R) -> TIM2_CH3 (PA2)
 *   绿(G) -> TIM2_CH1 (PA15)
 *   蓝(B) -> TIM2_CH2 (PB3)
 *
 * 演示流程：红 -> 绿 -> 蓝 -> 白 各 1 s，随后自动进入彩虹渐变循环。
 */
#ifndef APP_COLOR_LED_H
#define APP_COLOR_LED_H

#include <stdint.h>

/** 启动 TIM2 三通道 PWM，初始输出关闭（全灭） */
void app_color_led_init(void);
/** 软定时器回调（20 ms 周期）：驱动演示状态机 */
void app_color_led_on_timer_tick(void);
/** 主循环：有更新标志时刷新颜色输出与 OLED */
void app_color_led_poll(void);
/** 整屏：标题 + 当前颜色值 */
void app_color_led_ui_full(void);

#endif /* APP_COLOR_LED_H */
