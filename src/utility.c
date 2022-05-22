#include "utility.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

char *strnfindl(const char *str, char *chs, size_t n_ch, size_t n)
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
