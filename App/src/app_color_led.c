/**
 * @file app_color_led.c
 * @brief PWM 三色 LED 演示实现（接口说明见 app_color_led.h）
 */
#include "app_color_led.h"

#include "app_config.h"
#include "driver_oled.h"
#include "tim.h"

/* 通道映射：与 CubeMX 生成的 TIM2 GPIO 配置一致 */
#define COLOR_CH_R  TIM_CHANNEL_3   /* PA2  */
#define COLOR_CH_G  TIM_CHANNEL_1   /* PA15 */
#define COLOR_CH_B  TIM_CHANNEL_2   /* PB3  */

/* CubeMX 中 TIM2 的 ARR 值 */
#define TIM2_ARR    1999u

/* 演示阶段 */
#define PHASE_RED     0
#define PHASE_GREEN   1
#define PHASE_BLUE    2
#define PHASE_WHITE   3
#define PHASE_GRADIENT 4

/* 固定颜色停留时间：50 ticks * 20 ms = 1 s */
#define FIXED_COLOR_TICKS  50u
/* 渐变色相总步数（0-767 覆盖 R->G->B->R 一圈） */
#define HUE_STEPS  768u

static volatile uint8_t  s_update_flag;
static uint8_t  s_phase;
static uint16_t s_tick_cnt;
static uint16_t s_hue;
static uint32_t s_cur_color;

/* ---------- 底层：设置 RGB 颜色 ---------- */

static void color_led_set(uint32_t color)
{
    uint32_t r = (color >> 16) & 0xFFu;
    uint32_t g = (color >>  8) & 0xFFu;
    uint32_t b =  color        & 0xFFu;

    __HAL_TIM_SET_COMPARE(&htim2, COLOR_CH_R, r * TIM2_ARR / 255u);
    __HAL_TIM_SET_COMPARE(&htim2, COLOR_CH_G, g * TIM2_ARR / 255u);
    __HAL_TIM_SET_COMPARE(&htim2, COLOR_CH_B, b * TIM2_ARR / 255u);

    s_cur_color = color;
}

/* 简易色相环：hue 0-767 -> RGB（每段 256 步） */
static uint32_t hue_to_rgb(uint16_t hue)
{
    uint8_t r, g, b;
    uint16_t seg = hue / 256u;
    uint8_t  t   = (uint8_t)(hue % 256u);

    switch (seg) {
    case 0:  r = (uint8_t)(255u - t); g = t;                   b = 0;                   break;
    case 1:  r = 0;                   g = (uint8_t)(255u - t);  b = t;                   break;
    default: r = t;                   g = 0;                   b = (uint8_t)(255u - t);  break;
    }
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

/* ---------- OLED 显示辅助 ---------- */

static void draw_color_info(void)
{
    uint8_t r = (uint8_t)((s_cur_color >> 16) & 0xFFu);
    uint8_t g = (uint8_t)((s_cur_color >>  8) & 0xFFu);
    uint8_t b = (uint8_t)( s_cur_color        & 0xFFu);
    char buf[16];

    /* 行 1："#RRGGBB" */
    buf[0] = '#';
    buf[1] = "0123456789ABCDEF"[r >> 4];
    buf[2] = "0123456789ABCDEF"[r & 0xF];
    buf[3] = "0123456789ABCDEF"[g >> 4];
    buf[4] = "0123456789ABCDEF"[g & 0xF];
    buf[5] = "0123456789ABCDEF"[b >> 4];
    buf[6] = "0123456789ABCDEF"[b & 0xF];
    buf[7] = '\0';
    OLED_PrintString(OLED_X_TEXT, OLED_Y_BODY, buf);

    /* 行 2："Rxx Gxx Bxx" 十进制各分量 */
    int i = 0;
    buf[i++] = 'R';
    buf[i++] = (char)('0' + r / 100u);
    buf[i++] = (char)('0' + (r / 10u) % 10u);
    buf[i++] = (char)('0' + r % 10u);
    buf[i++] = ' ';
    buf[i++] = 'G';
    buf[i++] = (char)('0' + g / 100u);
    buf[i++] = (char)('0' + (g / 10u) % 10u);
    buf[i++] = (char)('0' + g % 10u);
    buf[i++] = ' ';
    buf[i++] = 'B';
    buf[i++] = (char)('0' + b / 100u);
    buf[i++] = (char)('0' + (b / 10u) % 10u);
    buf[i++] = (char)('0' + b % 10u);
    buf[i]   = '\0';
    OLED_PrintString(OLED_X_TEXT, OLED_Y_BODY + 2u, buf);
}

/* ---------- 公开接口 ---------- */

void app_color_led_init(void)
{
    s_phase       = PHASE_RED;
    s_tick_cnt    = 0;
    s_hue         = 0;
    s_update_flag = 0;
    s_cur_color   = 0;

    HAL_TIM_PWM_Start(&htim2, COLOR_CH_R);
    HAL_TIM_PWM_Start(&htim2, COLOR_CH_G);
    HAL_TIM_PWM_Start(&htim2, COLOR_CH_B);

    color_led_set(0x00FF0000u);   /* 从红色开始 */
    s_update_flag = 1;
}

void app_color_led_on_timer_tick(void)
{
#if (APP_MODE_SELECT != APP_MODE_COLOR_LED)
    return;
#endif
    s_tick_cnt++;

    if (s_phase < PHASE_GRADIENT) {
        if (s_tick_cnt >= FIXED_COLOR_TICKS) {
            s_tick_cnt = 0;
            s_phase++;
            s_update_flag = 1;
        }
    } else {
        /* 渐变：每 tick 步进 2 */
        s_hue = (s_hue + 2u) % HUE_STEPS;
        s_update_flag = 1;
    }
}

void app_color_led_poll(void)
{
#if (APP_MODE_SELECT != APP_MODE_COLOR_LED)
    return;
#endif
    if (!s_update_flag) {
        return;
    }
    s_update_flag = 0;

    switch (s_phase) {
    case PHASE_RED:     color_led_set(0x00FF0000u); break;
    case PHASE_GREEN:   color_led_set(0x0000FF00u); break;
    case PHASE_BLUE:    color_led_set(0x000000FFu); break;
    case PHASE_WHITE:   color_led_set(0x00FFFFFFu); break;
    default:            color_led_set(hue_to_rgb(s_hue)); break;
    }

    draw_color_info();
}

void app_color_led_ui_full(void)
{
    OLED_PrintString(OLED_X_TEXT, OLED_Y_TITLE, "PWM ColorLED");
    draw_color_info();
}
