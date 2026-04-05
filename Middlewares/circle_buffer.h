#ifndef __CIRCLE_BUFFER_H
#define __CIRCLE_BUFFER_H

#include <stdint.h>

// 环形队列结构体
typedef struct {
    volatile uint32_t read_index; // 读位置
    volatile uint32_t write_index; // 写位置
    uint8_t *data; // 缓冲区
    uint32_t len; // 缓冲区大小
} circle_buffer_t;

// 初始化环形队列
void circle_buffer_init(circle_buffer_t *buffer, uint8_t *data, uint32_t len);
// 写入环形队列
int circle_buffer_write(circle_buffer_t *buffer, uint8_t value);
// 读取环形队列
int circle_buffer_read(circle_buffer_t *buffer, uint8_t *value);
// 获取环形队列大小
int circle_buffer_get_size(circle_buffer_t *buffer);
// 获取环形队列剩余大小
int circle_buffer_get_remaining(circle_buffer_t *buffer);

#endif /* __CIRCLE_BUFFER_H */
