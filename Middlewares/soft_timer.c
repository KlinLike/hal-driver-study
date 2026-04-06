#include "soft_timer.h"
#include "stm32f1xx_hal.h"

void soft_timer_init(soft_timer_t *timer, uint32_t timeout_ms, uint8_t mode,
                     soft_timer_callback_t callback, void *arg)
{
    timer->timeout_ms = timeout_ms;
    timer->start_tick = 0;
    timer->callback = callback;
    timer->arg = arg;
    timer->mode = mode;
    timer->running = 0;
}

void soft_timer_start(soft_timer_t *timer)
{
    timer->start_tick = HAL_GetTick();
    timer->running = 1;
}

void soft_timer_stop(soft_timer_t *timer)
{
    timer->running = 0;
}

int soft_timer_is_running(soft_timer_t *timer)
{
    return timer->running;
}

void soft_timer_process(soft_timer_t *timer)
{
    if (!timer->running)
        return;

    // 无符号减法天然处理 tick 溢出
    if ((HAL_GetTick() - timer->start_tick) >= timer->timeout_ms) {
        if (timer->callback)
            timer->callback(timer->arg);

        if (timer->mode == SOFT_TIMER_MODE_PERIODIC)
            timer->start_tick += timer->timeout_ms;  // 用 += 防止周期漂移
        else
            timer->running = 0;  // 单次模式到期后自动停止
    }
}
