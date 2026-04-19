/**
 * @file app_board.h
 * @brief 板级 GPIO：LED、蜂鸣器、光敏等（与 main.h 中引脚命名一致）
 */
#ifndef APP_BOARD_H
#define APP_BOARD_H

/** LED：on 非 0 点亮（低电平有效硬件时内部写 RESET） */
void LED_Control(int on);
/** 蜂鸣器：on 非 0 鸣叫 */
void BUZZ_Control(int on);
/** 光敏：返回 1 表示“触发/低亮度”等与电路一致的逻辑（见实现） */
int LDR_Status(void);

#endif /* APP_BOARD_H */
