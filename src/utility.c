#define _LARGEFILE64_SOURCE
#include "buffer.h"
#include "utility.h"
#include "log.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int was_signaled(int code)
{
    return code == -1 && errno == EINTR;
}

int is_space(char ch)
{
    return ch == ' '  || ch == '\t';
}

int min(int lhs, int rhs)
{
    return lhs > rhs ? rhs : lhs;
}

int max(int lhs, int rhs)
{
    return lhs > rhs ? lhs : rhs;
}

char *strdup(char *src)
{
    size_t len;
    char *cpy = NULL;
    if (src)
    {
        len = strlen(src) + 1;
        cpy = (char *) malloc(len);
        ANLOG(cpy, len);
        strcpy(cpy, src);
    }
    return cpy;
}

char *strndup(char *src, size_t n)
{
    char *cpy = NULL;
    if (src)
    {
        cpy = (char *) malloc(n);
        ANLOG(cpy, n);
        strncpy(cpy, src, n);
    }
    return cpy;
}

size_t strncount(const char *str, char ch, size_t n)
{
    size_t count = 0;
    for (; n; str++, n--)
        if (*str == ch)
            count++;
    return count;
}

int stricmp(const char *lhs, const char *rhs)
{
    for (; *lhs && *rhs; lhs++, rhs++)
        if (tolower(*lhs) != tolower(*rhs))
            return *lhs - *rhs;
    return 0;
}

int strnicmp(const char *lhs, const char *rhs, size_t n)
{
    for (; n; n--, lhs++, rhs++)
        if (tolower(*lhs) != tolower(*rhs))
            return *lhs - *rhs;
    return 0;
}

char *strfind(const char *str, char ch)
{
    for (; *str; str++)
        if (*str == ch)
            return (char *) str;
    return *str == ch ? (char *) str : NULL;
}

char *strrfind(const char *str, char ch)
{
    for (; *str; str--)
        if (*str == ch)
            return (char *) str;
    return *str == ch ? (char *) str : NULL;
}

char *strnfind(const char *str, char ch, size_t n)
{
    for (; n; n--, str++)
        if (*str == ch)
            return (char *) str;
    return NULL;
}

char *strrnfind(const char *str, char ch, size_t n)
{
    for (; n; n--, str--)
    {
        LOG("%lu\n", n);
        if (*str == ch)
        {
            LOG("return\n");
            return (char *) str;
        }
    }
    return NULL;
}

char *strfindl(const char *str, const char *chs, size_t n_ch)
{
    size_t j;
    for (; str; str++)
    {
        for (j = 0; j < n_ch; j++)
            if (*str == chs[j])
                return (char *) str;
    }
    return NULL;
}

char *strnfindl(const char *str, const char *chs, size_t n_ch, size_t n)
{
    size_t j;
    for (; n; n--, str++)
    {
        for (j = 0; j < n_ch; j++)
            if (*str == chs[j])
                return (char *) str;
    }
    return NULL;
}

char *strfindnth(const char *str, char ch, size_t nth)
{
    size_t found = 0;

    if (!nth)
        return (char *) str;
    for (; *str; str++)
    {
        if (*str == ch)
        {
            found++;
            if (found == nth)
                return (char *) str;
        }
    }
    return *str == ch ? (char *) str : NULL;
}

char *strnfindnth(const char *str, char ch, size_t nth, size_t n)
{
    size_t found = 0;

    if (!nth)
        return (char *) str;
    for (; n; n--, str++)
    {
        if (*str == ch)
        {
            found++;
            if (found == nth)
                return (char *) str;
        }
    }
    return NULL;
}

int file_exists(const char *path)
{
    int fd = open_file(path, O_RDONLY, 0);
    close(fd);
    return fd != -1;
}

int open_file(const char *path, int flag, int perms)
{
    int fd = open(path, flag, perms);
    if (fd == -1)
        PELOG("Failed to open a file");
    return fd;
}

size_t get_file_len(int fd)
{
    long long cur_pos = lseek64(fd, 0, SEEK_CUR);
    if (cur_pos == -1)
        PELOG_EX("Failed to lseek");
    lseek(fd, 0, SEEK_SET);
    size_t len = lseek64(fd, 0, SEEK_END);
    lseek(fd, cur_pos, 0);
    return len;
}

void print_byte(char byte)
{
	int i = 0;
	for (; i < 8; i++)
		putchar(!!(byte & (1 << i)) + '0');
}

void print_bits(const void *data, size_t bytes)
{
    assert(!(bytes % sizeof(char)));
	for (; bytes; bytes--, data += sizeof(char))
	{
		print_byte(*((char *) data));
		putchar(10);
	}
}

void print_buf_bits(const buf_t *buf)
{
    print_bits(buf->ptr, buf->used);
}

char *find_space_n(const char *buf, size_t n)
{
    const char delims[] = { ' ', '\t' };
    return strnfindl(buf, delims, sizeof(delims), n);
}

char *skip_spaces_n(const char *buf, size_t n)
{
    if (!(buf = find_space_n(buf, n)))
        return NULL;
    for (; n && is_space(*buf); n--, buf++)
        {   }
    return (char *) buf;
}
