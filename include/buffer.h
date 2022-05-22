#ifndef BUFFER_H
#define BUFFER_H

#include "def.h"

#include <sys/types.h>

/*
 * Buffer is a simple struct that holds a raw pointer to some data which has
 * to be cast to the right type. It's useful due its ability to remember how
 * many bytes have been used and the overal size.
 */

struct buffer
{
    void *ptr;
    const size_t size;
    size_t used;
    size_t last_read_bytes;
};

buffer *buffer_create(size_t size);
void buffer_delete(buffer *buf);
void buffer_set_size(buffer *buf, size_t size);
void buffer_append(buffer *buf, const char *data, size_t bytes);
void buffer_move_left(buffer *buf, size_t bytes);
void buffer_move_right(buffer *buf, size_t bytes);
void buffer_clear(buffer *buf);

#endif /* !BUFFER_H */
