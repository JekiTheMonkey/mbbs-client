#include "buffer.h"
#include "log.h"
#include "utility.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

buf_t *buffer_create(size_t size)
{
    buf_t *buf = (buf_t *) malloc(sizeof(buf_t));
    ALOG(buf);
    buf->ptr = (void *) malloc(size);
    ANLOG(buf->ptr, size);
    buf->used = 0;
    buffer_set_size(buf, size);
    buf->last_read_bytes = 0;
    return buf;
}

void buffer_delete(buf_t *buf)
{
    FREE(buf->ptr);
}

void buffer_set_size(buf_t *buf, size_t size)
{
    size_t *s_p = (size_t *) &buf->size;
    *s_p = size;
    LOG("'%lu' has been set as buffer size\n", size);
}

void buffer_append(buf_t *buf, const char *data, size_t bytes)
{
    const size_t rem = buf->size - buf->used;
    LOG("Remained %lu To append %lu\n", rem, bytes);
    assert(rem > bytes);
    memmove(buf->ptr + buf->used, data, bytes);
    buf->used += bytes;
    LOG("'%lu' bytes have been appended\n", bytes);
}

void buffer_move_left(buf_t *buf, size_t bytes)
{
    assert(buf->used >= bytes);
    const size_t diff = buf->used - bytes;
    memmove(buf->ptr, buf->ptr + bytes, diff);
    memset(buf->ptr + diff, 0, bytes);
    buf->used = diff;
    LOG("'%lu' bytes have been moved to left\n", diff);
}

void buffer_move_right(buf_t *buf, size_t bytes)
{
    assert(buf->used >= bytes);
    const size_t diff = buf->used - bytes;
    memset(buf->ptr + diff, 0, bytes);
    buf->used = diff;
    LOG("'%lu' bytes have been moved from right\n", diff);
}

void buffer_clear(buf_t *buf)
{
    memset(buf->ptr, 0, buf->used);
    buf->used = 0;
    LOG("Buffer has been cleaned up\n");
}

void buffer_print_bin(const buf_t *buf)
{
    size_t n = buf->used;
    const char *data = buf->ptr;
    for (; n; n--, data++)
        printf("%c | %d\n", *data > 32 ? *data : ' ', *data);
}

size_t buffer_count_argc(const buf_t *buf)
{
    size_t argc = 0;
    size_t n = buf->used;
    const char *ch_buf = (char *) buf->ptr;
    const char *it;
    LOG("'%.*s'\n", (unsigned) buf->used, (char *) buf->ptr);
    for (; ch_buf; argc++, ch_buf = it)
    {
        it = skip_spaces_n(ch_buf, n);
        if (it)
            n -= it - ch_buf;
        if (!n)
            return argc;
    }
    return argc;
}

char *buffer_get_argv_n(buf_t *buf, size_t n)
{
    assert(buffer_count_argc(buf) >= n);
    n--; /* n = arguments to skip */
    char *it, *data = buf->ptr;
    size_t bytes = buf->used;
    for (; n; n--)
    {
        it = skip_spaces_n(data, bytes);
        bytes -= it - data;
        data = it;
    }
    return data;
}

int buffer_appendf(buf_t *buf, const char *format, ...)
{
    va_list vl;
    va_start(vl, format);
    int res = vsprintf((char *) buf->ptr + buf->used, format, vl);
    LOG("Added bytes: '%d'\n", res);
    va_end(vl);
    buf->used += res;
    assert(buf->used + res <= buf->size);
    return res;
}
