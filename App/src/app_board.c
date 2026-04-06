/**
 * @file app_board.c
 * @brief 板级 GPIO 实现（接口说明见 app_board.h）
 */
#include "app_board.h"

#include "main.h"

/** LED：on 非 0 点亮（低电平有效硬件时内部写 RESET） */
void LED_Control(int on)
{
    if (on) {
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_GPIO_Pin, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_GPIO_Pin, GPIO_PIN_SET);
    }
}

/** 蜂鸣器：on 非 0 鸣叫 */
void BUZZ_Control(int on)
{
    if (on) {
        HAL_GPIO_WritePin(BUZZ_GPIO_Port, BUZZ_GPIO_Pin, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(BUZZ_GPIO_Port, BUZZ_GPIO_Pin, GPIO_PIN_SET);
    }
}

/** 光敏：返回 1 表示“触发/低亮度”等与电路一致的逻辑（见实现） */
int LDR_Status(void)
{
    return (HAL_GPIO_ReadPin(LDR_GPIO_Port, LDR_GPIO_Pin) == GPIO_PIN_RESET);
}
