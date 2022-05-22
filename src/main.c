#include "client.h"

#include <stdio.h>

/* void handle_lf(buffer *buf, int wfd) */
/* { */
/*     int lf = strnfind((char *) buf->ptr, '\n', buf->used); */
/*     if (lf == -1) */
/*         return; */
/*     lf++; /1* index to bytes *1/ */
/*     write(wfd, buf->buf, lf); */
/*     memmove(buf->buf, buf->buf + lf, buf->used - lf); */
/*     buf->used -= lf; */
/* } */

/* int handle_read(int rfd, int wfd, const fd_set *readfds, buf_t *buf) */
/* { */
/*     if (FD_ISSET(rfd, readfds)) */
/*     { */
/*         int res = read(rfd, buf->buf + buf->used, buf->size - buf->used); */
/*         if (res == -1) */
/*             perror("Failed to read"); */
/*         if (res <= 0) */
/*             return 0; */

/*         buf->used += res; */
/*         handle_lf(buf, wfd); */
/*         if (buf->used == buf->size) */
/*         { */
/*             fprintf(stderr, "A buffer is full\n"); */
/*             return 0; */
/*         } */
/*     } */
/*     return 1; */
/* } */

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Usage: ./mbbs-client <IP> <port>\n");
        return 0;
    }

    client cli;
    client_init(&cli, argv);
    return client_start(&cli);
}
