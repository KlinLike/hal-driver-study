#ifndef __SOFT_TIMER_H
#define __SOFT_TIMER_H

#include <stdint.h>

#define SOFT_TIMER_MODE_ONESHOT  0  // 单次模式：到期后自动停止
#define SOFT_TIMER_MODE_PERIODIC 1  // 周期模式：到期后自动重装

// 定时器回调函数类型
typedef void (*soft_timer_callback_t)(void *arg);

// 软定时器结构体
typedef struct {
    uint32_t timeout_ms;            // 超时时间（毫秒）
    uint32_t start_tick;            // 本轮计时的起始 tick
    soft_timer_callback_t callback; // 到期回调，可为 NULL
    void *arg;                      // 传给回调的用户参数
    uint8_t mode;                   // ONESHOT 或 PERIODIC
    uint8_t running;                // 1=运行中 0=已停止
} soft_timer_t;

// 初始化定时器（不会自动启动）
void soft_timer_init(soft_timer_t *timer, uint32_t timeout_ms, uint8_t mode,
                     soft_timer_callback_t callback, void *arg);
// 启动定时器
void soft_timer_start(soft_timer_t *timer);
// 停止定时器
void soft_timer_stop(soft_timer_t *timer);
// 查询是否在运行
int  soft_timer_is_running(soft_timer_t *timer);
// 在主循环中调用，检查是否到期并触发回调
void soft_timer_process(soft_timer_t *timer);

#endif /* __SOFT_TIMER_H */
