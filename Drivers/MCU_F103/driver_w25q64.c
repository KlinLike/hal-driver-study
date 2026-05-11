/**
 * @file driver_w25q64.c
 * @brief W25Q64 SPI Flash 驱动实现（课程 049-050 配套）
 *
 * 使用 SPI 中断方式（_IT 函数）：
 *   1) 调用 HAL_SPI_xxx_IT() 启动传输，函数立即返回
 *   2) SPI 硬件每传完一字节触发中断，HAL 在中断里自动处理后续字节
 *   3) 全部字节传完后，HAL 调用回调函数（TxCplt / RxCplt / TxRxCplt）
 *   4) 回调里置标志位，主线程等待标志位变为 1 后继续
 *
 * 这种"启动 + 等待标志"的模式在 app_mpu6050.c 的中断方式中也用过，
 * 只不过那里是 I2C，这里是 SPI。
 */
#include "driver_w25q64.h"

#include "main.h"
#include "spi.h"

/* ====================================================================== */
/*  W25Q64 命令字（芯片手册 Instruction Set Table）                       */
/* ====================================================================== */
#define W25Q64_CMD_WRITE_ENABLE   0x06u  /* 写使能：每次擦除/写入前必须先发 */
#define W25Q64_CMD_READ_STATUS1   0x05u  /* 读状态寄存器 1：判断 BUSY */
#define W25Q64_CMD_READ_DATA      0x03u  /* 读数据：命令 + 3 字节地址 + 数据流出 */
#define W25Q64_CMD_PAGE_PROGRAM   0x02u  /* 页编程：命令 + 3 字节地址 + 数据流入 */
#define W25Q64_CMD_SECTOR_ERASE   0x20u  /* 扇区擦除(4KB)：命令 + 3 字节地址 */
#define W25Q64_CMD_READ_JEDEC_ID  0x9Fu  /* 读 JEDEC ID：命令 + 3 字节 ID 流出 */

/* ====================================================================== */
/*  SPI 中断方式的核心机制：完成标志 + 回调 + 等待函数                    */
/* ====================================================================== */

/*
 * 为什么要用 volatile？
 * 这些标志在中断回调（ISR）中被置 1，在主线程的 while 循环中被读取。
 * 如果不加 volatile，编译器可能优化成"只读一次就缓存到寄存器"，
 * 导致主线程永远看不到中断里的修改，陷入死循环。
 *
 * 和 app_mpu6050.c 中的 s_tx_done / s_rx_done 原理完全相同。
 */
static volatile uint8_t s_spi_tx_done;
static volatile uint8_t s_spi_rx_done;
static volatile uint8_t s_spi_txrx_done;

/*
 * HAL 回调函数：SPI 传输完成时由中断上下文调用。
 * 函数名是 HAL 库弱定义（__weak）的，我们重新定义就会覆盖默认空实现。
 *
 * 注意：一个项目中同一个回调只能定义一次。如果将来有多个 SPI 设备，
 * 需要通过 hspi 指针判断是哪个 SPI 实例（这里只有 SPI1）。
 */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == &hspi1)
        s_spi_tx_done = 1u;
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == &hspi1)
        s_spi_rx_done = 1u;
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == &hspi1)
        s_spi_txrx_done = 1u;
}

/*
 * 等待函数：带超时保护。
 * 课程示例用的是 while(!flag); 死等，实际工程中如果 SPI 硬件出问题会卡死。
 * 这里加了 HAL_GetTick() 超时检测，和 app_mpu6050.c 的 wait_tx_done_ms() 思路一样。
 */
#define SPI_WAIT_TIMEOUT_MS  1000u

static void Wait_SPI_Tx_Complete(void)
{
    uint32_t t0 = HAL_GetTick();
    while (!s_spi_tx_done) {
        if ((HAL_GetTick() - t0) > SPI_WAIT_TIMEOUT_MS)
            break;
    }
    s_spi_tx_done = 0u;
}

static void Wait_SPI_Rx_Complete(void)
{
    uint32_t t0 = HAL_GetTick();
    while (!s_spi_rx_done) {
        if ((HAL_GetTick() - t0) > SPI_WAIT_TIMEOUT_MS)
            break;
    }
    s_spi_rx_done = 0u;
}

static void Wait_SPI_TxRx_Complete(void)
{
    uint32_t t0 = HAL_GetTick();
    while (!s_spi_txrx_done) {
        if ((HAL_GetTick() - t0) > SPI_WAIT_TIMEOUT_MS)
            break;
    }
    s_spi_txrx_done = 0u;
}

/* ====================================================================== */
/*  内部函数（static）：片选、写使能、状态读取、等待就绪                  */
/* ====================================================================== */

/*
 * 片选控制：SPI Flash 的 CS 是低电平有效。
 * 每次 SPI 通信前拉低（选中），通信结束后拉高（释放）。
 * 同一时刻只能选中一个 SPI 设备。
 */
static void SPI_Flash_Select(void)
{
    HAL_GPIO_WritePin(SPI_FLASH_CS_GPIO_Port, SPI_FLASH_CS_Pin, GPIO_PIN_RESET);
}

static void SPI_Flash_DeSelect(void)
{
    HAL_GPIO_WritePin(SPI_FLASH_CS_GPIO_Port, SPI_FLASH_CS_Pin, GPIO_PIN_SET);
}

/*
 * 写使能：发送命令 0x06。
 * W25Q64 每次擦除或写入操作前都必须先发写使能，操作完成后 WEL 位自动清零。
 * 这是 Flash 的安全机制，防止误写。
 */
static void Write_Enable(void)
{
    uint8_t cmd = W25Q64_CMD_WRITE_ENABLE;

    SPI_Flash_Select();
    HAL_SPI_Transmit_IT(&hspi1, &cmd, 1u);
    Wait_SPI_Tx_Complete();
    SPI_Flash_DeSelect();
}

/*
 * 读状态寄存器 1：发送命令 0x05 + 一个假字节，接收第 2 字节即为状态值。
 *
 * 【SPI 全双工的核心理解】：
 * SPI 发送 N 字节的同时，必然会接收 N 字节。
 * 这里发 2 字节（命令 + 假数据 0xFF），同时收 2 字节。
 * rx_buf[0] 是发命令期间收到的无意义数据（Flash 还没响应），
 * rx_buf[1] 才是 Flash 返回的状态寄存器值。
 */
static uint8_t Read_Status(void)
{
    uint8_t tx_buf[2] = {W25Q64_CMD_READ_STATUS1, 0xFFu};
    uint8_t rx_buf[2] = {0u, 0u};

    SPI_Flash_Select();
    HAL_SPI_TransmitReceive_IT(&hspi1, tx_buf, rx_buf, 2u);
    Wait_SPI_TxRx_Complete();
    SPI_Flash_DeSelect();

    return rx_buf[1];
}

/*
 * 等待 Flash 就绪：擦除和写入操作在 Flash 内部需要时间（擦除最慢，可达数百 ms）。
 * 通过反复读状态寄存器的 Bit0 (BUSY) 来等待：BUSY=1 表示正忙，BUSY=0 表示完成。
 */
static void Wait_Ready(void)
{
    while ((Read_Status() & 0x01u) != 0u) {
        HAL_Delay(1);
    }
}

/* ====================================================================== */
/*  对外函数：ReadID / EraseSector / Write / Read                         */
/* ====================================================================== */

/*
 * 读 JEDEC ID：验证 SPI 通信是否正常的第一步。
 * 发送命令 0x9F，随后 Flash 返回 3 字节：
 *   Byte1 = Manufacturer ID (Winbond = 0xEF)
 *   Byte2 = Memory Type     (0x40)
 *   Byte3 = Capacity        (W25Q64 = 0x17)
 * 组合为 0xEF4017。
 *
 * 同样利用 SPI 全双工特性：发 4 字节（1 命令 + 3 假数据），收 4 字节。
 */
uint32_t W25Q64_ReadID(void)
{
    uint8_t tx_buf[4] = {W25Q64_CMD_READ_JEDEC_ID, 0xFFu, 0xFFu, 0xFFu};
    uint8_t rx_buf[4] = {0u};

    SPI_Flash_Select();
    HAL_SPI_TransmitReceive_IT(&hspi1, tx_buf, rx_buf, 4u);
    Wait_SPI_TxRx_Complete();
    SPI_Flash_DeSelect();

    return ((uint32_t)rx_buf[1] << 16) |
           ((uint32_t)rx_buf[2] << 8)  |
           ((uint32_t)rx_buf[3]);
}

/*
 * 扇区擦除(4KB)：
 * 流程：写使能 -> CS拉低 -> 发送命令+24位地址 -> CS拉高 -> 等待Flash内部完成
 *
 * 24 位地址拆分成 3 字节：高 8 位、中 8 位、低 8 位，MSB 先发。
 * Flash 会擦除该地址所在的整个 4KB 扇区（地址会自动对齐到 4K 边界）。
 */
void W25Q64_EraseSector(uint32_t addr)
{
    uint8_t tx_buf[4];
    tx_buf[0] = W25Q64_CMD_SECTOR_ERASE;
    tx_buf[1] = (uint8_t)((addr >> 16) & 0xFFu);
    tx_buf[2] = (uint8_t)((addr >> 8) & 0xFFu);
    tx_buf[3] = (uint8_t)(addr & 0xFFu);

    Write_Enable();

    SPI_Flash_Select();
    HAL_SPI_Transmit_IT(&hspi1, tx_buf, 4u);
    Wait_SPI_Tx_Complete();
    SPI_Flash_DeSelect();

    Wait_Ready();
}

/*
 * 页编程（写数据）：
 * 流程：写使能 -> CS拉低 -> 命令+地址 -> 数据 -> CS拉高 -> 等待完成
 *
 * 注意 CS 必须在整个过程中保持低电平（命令 + 地址 + 数据 在同一个 CS 周期内）。
 * 所以先发命令+地址，等完成后再发数据，中间不能拉高 CS。
 *
 * len 最大 256 字节（一页），且写入不能跨页边界（地址到达页末尾会回绕到页开头）。
 */
void W25Q64_Write(uint32_t addr, const uint8_t *data, uint32_t len)
{
    uint8_t tx_buf[4];
    tx_buf[0] = W25Q64_CMD_PAGE_PROGRAM;
    tx_buf[1] = (uint8_t)((addr >> 16) & 0xFFu);
    tx_buf[2] = (uint8_t)((addr >> 8) & 0xFFu);
    tx_buf[3] = (uint8_t)(addr & 0xFFu);

    Write_Enable();

    SPI_Flash_Select();
    HAL_SPI_Transmit_IT(&hspi1, tx_buf, 4u);
    Wait_SPI_Tx_Complete();

    HAL_SPI_Transmit_IT(&hspi1, (uint8_t *)data, (uint16_t)len);
    Wait_SPI_Tx_Complete(); // 等待发送完成，但是不代表着写入完成了
    SPI_Flash_DeSelect(); // 取消片选，Flash就知道数据已经发完了

    Wait_Ready(); // 等待Flash内部完成写入操作 BUSY=0
}

/*
 * 读数据：
 * 流程：CS拉低 -> 命令+地址 -> 接收数据 -> CS拉高
 *
 * 读操作不需要写使能，也不需要等待就绪（读是即时的）。
 * 可以从任意地址开始，连续读取任意长度，会自动跨页/跨扇区。
 *
 * HAL_SPI_Receive_IT 内部会发送"假数据"来产生时钟，Flash 在时钟驱动下输出数据。
 */
void W25Q64_Read(uint32_t addr, uint8_t *data, uint32_t len)
{
    uint8_t tx_buf[4];
    tx_buf[0] = W25Q64_CMD_READ_DATA;
    tx_buf[1] = (uint8_t)((addr >> 16) & 0xFFu);
    tx_buf[2] = (uint8_t)((addr >> 8) & 0xFFu);
    tx_buf[3] = (uint8_t)(addr & 0xFFu);

    SPI_Flash_Select();
    HAL_SPI_Transmit_IT(&hspi1, tx_buf, 4u);
    Wait_SPI_Tx_Complete();

    HAL_SPI_Receive_IT(&hspi1, data, (uint16_t)len);
    Wait_SPI_Rx_Complete();
    SPI_Flash_DeSelect();
}
