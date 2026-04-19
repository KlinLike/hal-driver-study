#include "circle_buffer.h"

void circle_buffer_init(circle_buffer_t *buffer, uint8_t *data, uint32_t len)
{
    buffer->data = data;
    buffer->len = len;
    buffer->read_index = 0;
    buffer->write_index = 0;
}

int circle_buffer_write(circle_buffer_t *buffer, uint8_t value)
{
    uint32_t next = (buffer->write_index + 1) % buffer->len;
    if (next == buffer->read_index)
        return -1;

    buffer->data[buffer->write_index] = value;
    buffer->write_index = next;
    return 0;
}

int circle_buffer_read(circle_buffer_t *buffer, uint8_t *value)
{
    if (buffer->read_index == buffer->write_index)
        return -1;

    *value = buffer->data[buffer->read_index];
    buffer->read_index = (buffer->read_index + 1) % buffer->len;
    return 0;
}

int circle_buffer_get_size(circle_buffer_t *buffer)
{
    return (buffer->write_index - buffer->read_index + buffer->len) % buffer->len;
}

int circle_buffer_get_remaining(circle_buffer_t *buffer)
{
    return buffer->len - 1 - circle_buffer_get_size(buffer);
}
