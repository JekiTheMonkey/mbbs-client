#ifndef BUFFER_H
#define BUFFER_H

#include "def.h"

#include <sys/types.h>

/*
 * Buffer is a simple struct that holds a raw pointer to some data which has
 * to be cast to the right type. It's useful due its ability to remember how
 * many bytes have been used and the overal size.
 */

struct buf_t
{
    void *ptr;
    const size_t size;
    size_t used;
    size_t last_read_bytes;
};

buf_t *buffer_create(size_t size);
void buffer_delete(buf_t *buf);
void buffer_set_size(buf_t *buf, size_t size);
void buffer_append(buf_t *buf, const char *data, size_t bytes);
void buffer_move_left(buf_t *buf, size_t bytes);
void buffer_move_right(buf_t *buf, size_t bytes);
void buffer_clear(buf_t *buf);
void buffer_print_bin(const buf_t *buf);

#endif /* !BUFFER_H */
