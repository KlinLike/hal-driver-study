/**
 * @file app_mpu6050.c
 * @brief MPU6050 查询方式实现（接口与踩坑说明见 app_mpu6050.h）
 *
 * 课程 041：演示 HAL I2C 内存模式 API（HAL_I2C_Mem_Read / Mem_Write）
 * 完成对 MPU6050 的"唤醒 + 读 WHO_AM_I"。
 *
 * I2C1 已由 CubeMX 在 main 中通过 MX_I2C1_Init() 初始化，
 * 引脚 PB6=SCL / PB7=SDA，标准模式 100 kHz，开漏 + 外部上拉。
 *
 * 关于"读到 0x70 而不是 0x68" 的解释，请看 app_mpu6050.h 顶部注释。
 */
#include "app_mpu6050.h"

#include <stdio.h>

#include "app_config.h"
#include "driver_oled.h"
#include "i2c.h"
#include "main.h"

/* ---- I2C 从机地址（AD0=GND 时为 0x68；AD0=VCC 改成 0x69） ---- */
#define MPU6050_ADDR_7BIT       0x68u                                /* 协议规定的 7 位地址 */
#define MPU6050_ADDR_HAL        ((uint16_t)(MPU6050_ADDR_7BIT << 1)) /* HAL 要 8 位：左移腾出 R/W 位 */

/* ---- MPU6050 内部寄存器编号（手册 Register Map） ---- */
#define MPU6050_REG_PWR_MGMT_1  0x6Bu   /* 电源管理 1：写 0 清 SLEEP 位即唤醒芯片 */
#define MPU6050_REG_WHO_AM_I    0x75u   /* 设备 ID：只读，真 MPU6050 固定返回 0x68 */
#define MPU6050_WHO_EXPECT      0x68u   /* WHO_AM_I 的期望值 */

/* ---- HAL 阻塞调用的兜底超时 ---- */
#define MPU6050_I2C_TIMEOUT_MS  1000u   /* 1s 足够覆盖任何调试期异常 */

/** 模块内部状态：是否成功读到 WHO + 读到的字节值 */
static int     s_who_read_ok;   /* 1=I2C 读成功（值不一定 0x68） 0=I2C 通信失败 */
static uint8_t s_who_am_i;      /* 读到的 WHO_AM_I 字节，便于 OLED/串口排查 */

/** 演示用：初始化 + 读 WHO_AM_I（查询/阻塞方式） */
void app_mpu6050_init(void)
{
    HAL_StatusTypeDef st;
    uint8_t           wake = 0x00u;

    s_who_read_ok = 0;
    s_who_am_i    = 0x00u;

    /* 1) 唤醒：清 SLEEP 位。Mem_Write 内部完成"START -> 设备地址 -> 寄存器地址 -> 数据 -> STOP" */
    st = HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR_HAL, MPU6050_REG_PWR_MGMT_1,
                           I2C_MEMADD_SIZE_8BIT, &wake, 1u, MPU6050_I2C_TIMEOUT_MS);
    if (st != HAL_OK) {
        printf("[MPU6050] I2C 写 PWR_MGMT_1 失败(HAL=%lu)，查接线/上拉/供电\r\n",
               (unsigned long)st);
        return;
    }

    /* 2) 读 WHO_AM_I：Mem_Read 内部完成"START -> 设备地址(W) -> 寄存器地址 ->
     *    RESTART -> 设备地址(R) -> 接收 -> NACK -> STOP" */
    st = HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR_HAL, MPU6050_REG_WHO_AM_I,
                          I2C_MEMADD_SIZE_8BIT, &s_who_am_i, 1u, MPU6050_I2C_TIMEOUT_MS);
    if (st != HAL_OK) {
        printf("[MPU6050] I2C 读 WHO_AM_I 失败(HAL=%lu)\r\n", (unsigned long)st);
        return;
    }

    s_who_read_ok = 1;
    printf("[MPU6050] WHO_AM_I = 0x%02X (真 MPU6050 应为 0x%02X)\r\n",
           s_who_am_i, MPU6050_WHO_EXPECT);
    if (s_who_am_i != MPU6050_WHO_EXPECT) {
        printf("[MPU6050] I2C 通信正常，但芯片不是 6050（详见头文件踩坑表）。\r\n");
    }
}

/** 整屏：标题 + WHO 读值 + 状态 */
void app_mpu6050_ui_full(void)
{
    OLED_PrintString(OLED_X_TEXT, OLED_Y_TITLE,    "MPU6050 WHO");
    OLED_PrintString(OLED_X_TEXT, OLED_Y_BODY,     "WHO:");
    OLED_PrintHex   (OLED_X_TEXT + 5u, OLED_Y_BODY, s_who_am_i, 2u);

    if (!s_who_read_ok) {
        OLED_PrintString(OLED_X_TEXT, OLED_Y_BODY + 2u, "I2C: FAIL");
    } else if (s_who_am_i == MPU6050_WHO_EXPECT) {
        OLED_PrintString(OLED_X_TEXT, OLED_Y_BODY + 2u, "ID : OK");
    } else {
        OLED_PrintString(OLED_X_TEXT, OLED_Y_BODY + 2u, "ID : OTHER");
    }
}
