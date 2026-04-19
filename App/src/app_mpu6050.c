/**
 * @file app_mpu6050.c
 * @brief MPU6050 演示实现：查询(041) / 中断(042) 两种方式（接口与踩坑见 .h）
 *
 * 由 APP_MODE_SELECT 在编译期二选一：
 *   APP_MODE_MPU6050_POLL -> HAL_I2C_Mem_Read / Mem_Write（阻塞查询）
 *   APP_MODE_MPU6050_IT   -> HAL_I2C_Mem_Read_IT / Mem_Write_IT（中断异步）
 *
 * 业务流程相同：唤醒(写 PWR_MGMT_1=0) -> 读 WHO_AM_I -> OLED + 串口。
 * I2C1 由 CubeMX 在 main 通过 MX_I2C1_Init() 初始化（PB6=SCL/PB7=SDA, 100 kHz）。
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

/* ---- 阻塞调用的兜底超时；中断版的等待超时 ---- */
#define MPU6050_I2C_TIMEOUT_MS  1000u   /* 1s 足够覆盖任何调试期异常 */

/** 模块内部状态：是否成功读到 WHO + 读到的字节值（两种模式共用） */
static int     s_who_read_ok;   /* 1=I2C 读成功（值不一定 0x68） 0=I2C 通信失败 */
static uint8_t s_who_am_i;      /* 读到的 WHO_AM_I 字节，便于 OLED/串口排查 */

/* ============================================================ */
/*  课程 041：查询（阻塞）方式                                  */
/* ============================================================ */
#if (APP_MODE_SELECT == APP_MODE_MPU6050_POLL)

#define MPU_MODE_LABEL "Mode: POLL"

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
        printf("[MPU6050/POLL] 写 PWR_MGMT_1 失败(HAL=%lu)\r\n", (unsigned long)st);
        return;
    }

    /* 2) 读 WHO_AM_I：Mem_Read 内部完成"START -> 设备地址(W) -> 寄存器地址 ->
     *    RESTART -> 设备地址(R) -> 接收 -> NACK -> STOP" */
    st = HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR_HAL, MPU6050_REG_WHO_AM_I,
                          I2C_MEMADD_SIZE_8BIT, &s_who_am_i, 1u, MPU6050_I2C_TIMEOUT_MS);
    if (st != HAL_OK) {
        printf("[MPU6050/POLL] 读 WHO_AM_I 失败(HAL=%lu)\r\n", (unsigned long)st);
        return;
    }

    s_who_read_ok = 1;
    printf("[MPU6050/POLL] WHO_AM_I = 0x%02X (真 MPU6050 应为 0x%02X)\r\n",
           s_who_am_i, MPU6050_WHO_EXPECT);
    if (s_who_am_i != MPU6050_WHO_EXPECT) {
        printf("[MPU6050/POLL] I2C 通信正常，但芯片不是 6050（详见头文件踩坑表）\r\n");
    }
}

/* ============================================================ */
/*  课程 042：中断（异步）方式                                  */
/* ============================================================ */
#elif (APP_MODE_SELECT == APP_MODE_MPU6050_IT)

#define MPU_MODE_LABEL "Mode: IT"

/* 完成标志：volatile 是必须的——主循环和中断都会访问，
 * 不加 volatile 编译器可能把循环里的读优化成只读一次，永远等不到 1。 */
static volatile uint8_t s_tx_done = 0u;  /* MemTx 完成回调置 1 */
static volatile uint8_t s_rx_done = 0u;  /* MemRx 完成回调置 1 */
static volatile uint8_t s_i2c_err = 0u;  /* 错误回调置 1（如 NACK / 总线错误） */

/* ---- HAL 回调：传输完成 / 错误时被中断上下文调用 ----
 * 注意函数名是 HAL 弱定义的，重写即可覆盖；不要自己再去使能 NVIC 中断，
 * CubeMX 已经在 i2c.c 的 MspInit 里使能 I2C1_EV/ER。 */
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c == &hi2c1) {
        s_tx_done = 1u;
    }
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c == &hi2c1) {
        s_rx_done = 1u;
    }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c == &hi2c1) {
        s_i2c_err = 1u;
        /* 错误时也唤醒等待方，避免它一直死等完成标志 */
        s_tx_done = 1u;
        s_rx_done = 1u;
    }
}

/* ---- 等待完成（带超时；课程示例是 while(标志==0) 死等，调试期容易卡死） ----
 * 返回值：1=正常完成 / 0=超时或 I2C 错误 */
static uint8_t wait_tx_done_ms(uint32_t timeout_ms)
{
    uint32_t t0 = HAL_GetTick();
    while (s_tx_done == 0u) {
        if ((HAL_GetTick() - t0) > timeout_ms) {
            return 0u;
        }
    }
    s_tx_done = 0u;
    return (s_i2c_err == 0u) ? 1u : 0u;
}

static uint8_t wait_rx_done_ms(uint32_t timeout_ms)
{
    uint32_t t0 = HAL_GetTick();
    while (s_rx_done == 0u) {
        if ((HAL_GetTick() - t0) > timeout_ms) {
            return 0u;
        }
    }
    s_rx_done = 0u;
    return (s_i2c_err == 0u) ? 1u : 0u;
}

/** 演示用：初始化 + 读 WHO_AM_I（中断/异步方式） */
void app_mpu6050_init(void)
{
    HAL_StatusTypeDef st;
    uint8_t           wake = 0x00u;

    s_who_read_ok = 0;
    s_who_am_i    = 0x00u;
    s_tx_done = 0u;
    s_rx_done = 0u;
    s_i2c_err = 0u;

    /* 1) 唤醒：_IT 版本会立刻返回，真正的字节传输在 I2C1_EV 中断里完成。
     *    完成时 HAL 框架会调用 HAL_I2C_MemTxCpltCallback() 把 s_tx_done 置 1。 */
    st = HAL_I2C_Mem_Write_IT(&hi2c1, MPU6050_ADDR_HAL, MPU6050_REG_PWR_MGMT_1,
                              I2C_MEMADD_SIZE_8BIT, &wake, 1u);
    if (st != HAL_OK) {
        printf("[MPU6050/IT] 启动 Mem_Write_IT 失败(HAL=%lu)\r\n", (unsigned long)st);
        return;
    }
    if (!wait_tx_done_ms(MPU6050_I2C_TIMEOUT_MS)) {
        printf("[MPU6050/IT] 等待写完成超时/错误 (err=%u)\r\n", (unsigned)s_i2c_err);
        return;
    }

    /* 2) 读 WHO_AM_I：同上，立即返回，完成回调走 MemRxCplt */
    st = HAL_I2C_Mem_Read_IT(&hi2c1, MPU6050_ADDR_HAL, MPU6050_REG_WHO_AM_I,
                             I2C_MEMADD_SIZE_8BIT, &s_who_am_i, 1u);
    if (st != HAL_OK) {
        printf("[MPU6050/IT] 启动 Mem_Read_IT 失败(HAL=%lu)\r\n", (unsigned long)st);
        return;
    }
    if (!wait_rx_done_ms(MPU6050_I2C_TIMEOUT_MS)) {
        printf("[MPU6050/IT] 等待读完成超时/错误 (err=%u)\r\n", (unsigned)s_i2c_err);
        return;
    }

    s_who_read_ok = 1;
    printf("[MPU6050/IT] WHO_AM_I = 0x%02X (真 MPU6050 应为 0x%02X)\r\n",
           s_who_am_i, MPU6050_WHO_EXPECT);
    if (s_who_am_i != MPU6050_WHO_EXPECT) {
        printf("[MPU6050/IT] I2C 通信正常，但芯片不是 6050（详见头文件踩坑表）\r\n");
    }
}

#else
#define MPU_MODE_LABEL "Mode: ?"
void app_mpu6050_init(void) { /* 当前模式未启用 MPU6050，提供空实现保持链接通过 */ }
#endif

/* ============================================================ */
/*  OLED 整屏（两种模式共用，只是顶部 Mode: 标签不同）           */
/* ============================================================ */
void app_mpu6050_ui_full(void)
{
    OLED_PrintString(OLED_X_TEXT, OLED_Y_TITLE,    "MPU6050 WHO");
    OLED_PrintString(OLED_X_TEXT, OLED_Y_BODY,     "WHO:");
    OLED_PrintHex   (OLED_X_TEXT + 5u, OLED_Y_BODY, s_who_am_i, 2u);

    if (!s_who_read_ok) {
        OLED_PrintString(OLED_X_TEXT, OLED_Y_BODY + 2u, "I2C: FAIL ");
    } else if (s_who_am_i == MPU6050_WHO_EXPECT) {
        OLED_PrintString(OLED_X_TEXT, OLED_Y_BODY + 2u, "ID : OK   ");
    } else {
        OLED_PrintString(OLED_X_TEXT, OLED_Y_BODY + 2u, "ID : OTHER");
    }

    /* 第三行标注当前是查询版还是中断版，方便对比演示 */
    OLED_PrintString(OLED_X_TEXT, OLED_Y_BODY + 4u, MPU_MODE_LABEL);
}
