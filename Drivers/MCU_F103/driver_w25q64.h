/**
 * @file driver_w25q64.h
 * @brief W25Q64 SPI Flash 驱动（课程 048-050 配套）
 *
 * 程序分层（课程 045）：
 *   应用层  ->  本驱动（Flash 命令序列）  ->  HAL SPI（控制器）  ->  W25Q64 硬件
 *
 * 本驱动封装了 W25Q64 的常用操作：读 ID、擦除、写入、读取。
 * 内部使用 SPI 中断方式（_IT 函数），通过回调 + 标志位同步等待完成。
 *
 * 硬件连接（百问网 STM32F103 开发板）：
 *   PA5  = SPI1_SCK
 *   PA6  = SPI1_MISO (Flash DO)
 *   PA7  = SPI1_MOSI (Flash DI)
 *   PB9  = SPI_FLASH_CS (软件片选，低电平有效)
 */
#ifndef DRIVER_W25Q64_H
#define DRIVER_W25Q64_H

#include <stdint.h>

/**
 * 读取芯片 JEDEC ID（命令 0x9F）
 * @return 24 位 ID，W25Q64 应返回 0xEF4017
 *         (Manufacturer=0xEF, MemType=0x40, Capacity=0x17)
 */
uint32_t W25Q64_ReadID(void);

/**
 * 擦除一个 4KB 扇区（命令 0x20）
 * 擦除后该扇区所有字节变为 0xFF。
 * @param addr 扇区内任意地址（24 位），函数会擦除该地址所在的整个 4KB 扇区
 */
void W25Q64_EraseSector(uint32_t addr);

/**
 * 页编程：写数据到 Flash（命令 0x02）
 * @param addr  起始地址（24 位）
 * @param data  待写入数据
 * @param len   数据长度，不超过 256（一页），且不能跨页边界
 *
 * 注意：写入前必须先擦除；Flash 只能把 1 写成 0，不能把 0 写成 1。
 */
void W25Q64_Write(uint32_t addr, const uint8_t *data, uint32_t len);

/**
 * 读取 Flash 数据（命令 0x03）
 * @param addr  起始地址（24 位）
 * @param data  接收缓冲区
 * @param len   读取长度，可跨页/跨扇区，无长度限制
 */
void W25Q64_Read(uint32_t addr, uint8_t *data, uint32_t len);

#endif /* DRIVER_W25Q64_H */
