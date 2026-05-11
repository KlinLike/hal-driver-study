/**
 * @file app_spi_flash.h
 * @brief SPI Flash (W25Q64) 演示：读 ID + 擦写读验证（课程 048-050 配套）
 *
 * 测试流程：
 *   1) 读取芯片 JEDEC ID，验证 SPI 通信正常
 *   2) 擦除指定扇区
 *   3) 写入测试字符串
 *   4) 读回数据并与写入内容比较
 *   5) OLED + 串口输出结果
 */
#ifndef APP_SPI_FLASH_H
#define APP_SPI_FLASH_H

/** 初始化：读 ID + 擦写读测试，结果存于模块内部 */
void app_spi_flash_init(void);
/** 整屏：标题 + JEDEC ID + 读回数据 + 测试结果(OK/FAIL) */
void app_spi_flash_ui_full(void);

#endif /* APP_SPI_FLASH_H */
