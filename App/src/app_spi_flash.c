/**
 * @file app_spi_flash.c
 * @brief SPI Flash (W25Q64) 演示实现（接口说明见 .h）
 *
 * 测试流程：读 ID -> 擦除扇区 -> 写入字符串 -> 读回比较 -> OLED + 串口输出
 * SPI1 由 CubeMX 在 main 通过 MX_SPI1_Init() 初始化。
 */
#include "app_spi_flash.h"

#include <stdio.h>
#include <string.h>

#include "app_config.h"
#include "driver_oled.h"
#include "driver_w25q64.h"

/* W25Q64 的 JEDEC ID 期望值：Manufacturer=0xEF, Type=0x40, Capacity=0x17 */
#define W25Q64_JEDEC_ID_EXPECT  0xEF4017u

/* 测试使用的扇区地址（选第 1 个扇区：0x1000 = 4096，避开扇区 0） */
#define TEST_SECTOR_ADDR  0x1000u

/* 测试写入的字符串 */
static const char s_test_str[] = "www.100ask.net";

/* 模块内部状态 */
static uint32_t s_jedec_id;
static char     s_read_buf[32];
static int      s_id_ok;
static int      s_rw_ok;

void app_spi_flash_init(void)
{
    s_id_ok = 0;
    s_rw_ok = 0;
    s_jedec_id = 0u;
    memset(s_read_buf, 0, sizeof(s_read_buf));

    /* 1) 读 JEDEC ID —— 验证 SPI 通信的第一步 */
    s_jedec_id = W25Q64_ReadID();
    printf("[SPI Flash] JEDEC ID = 0x%06lX (expect 0x%06lX)\r\n",
           (unsigned long)s_jedec_id, (unsigned long)W25Q64_JEDEC_ID_EXPECT);

    if (s_jedec_id == W25Q64_JEDEC_ID_EXPECT) {
        s_id_ok = 1;
    } else {
        printf("[SPI Flash] ID mismatch! Check wiring.\r\n");
        return;
    }

    /* 2) 擦除测试扇区 */
    printf("[SPI Flash] Erasing sector @ 0x%06lX ...\r\n", (unsigned long)TEST_SECTOR_ADDR);
    W25Q64_EraseSector(TEST_SECTOR_ADDR);

    /* 3) 写入测试字符串（含末尾 '\0'） */
    printf("[SPI Flash] Writing: \"%s\"\r\n", s_test_str);
    W25Q64_Write(TEST_SECTOR_ADDR, (const uint8_t *)s_test_str, sizeof(s_test_str));

    /* 4) 读回并比较 */
    W25Q64_Read(TEST_SECTOR_ADDR, (uint8_t *)s_read_buf, sizeof(s_test_str));
    printf("[SPI Flash] Read back: \"%s\"\r\n", s_read_buf);

    if (memcmp(s_test_str, s_read_buf, sizeof(s_test_str)) == 0) {
        s_rw_ok = 1;
        printf("[SPI Flash] Read/Write test PASSED!\r\n");
    } else {
        printf("[SPI Flash] Read/Write test FAILED!\r\n");
    }
}

void app_spi_flash_ui_full(void)
{
    OLED_PrintString(OLED_X_TEXT, OLED_Y_TITLE, "SPI W25Q64");

    /* 第二行：JEDEC ID */
    OLED_PrintString(OLED_X_TEXT, OLED_Y_BODY, "ID:");
    OLED_PrintHex(OLED_X_TEXT + 3u, OLED_Y_BODY, s_jedec_id, 2u);

    /* 第三行：读回的数据（截取前 13 字符以适应 OLED 宽度） */
    if (s_rw_ok) {
        OLED_PrintString(OLED_X_TEXT, OLED_Y_BODY + 2u, s_read_buf);
    } else if (s_id_ok) {
        OLED_PrintString(OLED_X_TEXT, OLED_Y_BODY + 2u, "RW: FAIL  ");
    } else {
        OLED_PrintString(OLED_X_TEXT, OLED_Y_BODY + 2u, "ID: FAIL  ");
    }

    /* 第四行：总结状态 */
    if (s_id_ok && s_rw_ok) {
        OLED_PrintString(OLED_X_TEXT, OLED_Y_BODY + 4u, "Test: OK  ");
    } else {
        OLED_PrintString(OLED_X_TEXT, OLED_Y_BODY + 4u, "Test: FAIL");
    }
}
