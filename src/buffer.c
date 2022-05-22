#include "buffer.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

buffer *buffer_create(size_t size)
{
    buffer *buf = (buffer *) malloc(sizeof(buffer));
    ALOG(buf);
    buf->ptr = (void *) malloc(size);
    ANLOG(buf->ptr, size);
    buf->used = 0;
    buffer_set_size(buf, size);
    buf->last_read_bytes = 0;
    return buf;
}

void buffer_delete(buffer *buf)
{
    FREE(buf->ptr);
}

void buffer_set_size(buffer *buf, size_t size)
{
    size_t *s_p = (size_t *) &buf->size;
    *s_p = size;
    LOG("'%lu' has been set as buffer size\n", size);
}

void buffer_append(buffer *buf, const char *data, size_t bytes)
{
    const size_t rem = buf->size - buf->used;
    assert(rem > bytes);
    memmove(buf->ptr + buf->used, data, bytes);
    buf->used += bytes;
    LOG("'%lu' bytes have been appended\n", bytes);
}

void buffer_move_left(buffer *buf, size_t bytes)
{
    assert(buf->used >= bytes);
    const size_t diff = buf->used - bytes;
    memmove(buf->ptr, buf->ptr + bytes, diff);
    memset(buf->ptr + diff, 0, bytes);
    buf->used = diff;
    LOG("'%lu' bytes have been moved to left\n", diff);
}

void buffer_move_right(buffer *buf, size_t bytes)
{
    assert(buf->used >= bytes);
    const size_t diff = buf->used - bytes;
    memset(buf->ptr + diff, 0, bytes);
    buf->used = diff;
    LOG("'%lu' bytes have been moved from right\n", diff);
}

void buffer_clear(buffer *buf)
{
    memset(buf->ptr, 0, buf->used);
    buf->used = 0;
    LOG("Buffer has been cleaned up\n");
}
