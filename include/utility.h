#ifndef UTILITY_H
#define UTILITY_H

#include "def.h"

#include <sys/types.h>

#define STR(x) #x

int was_signaled(int code);
int is_space(char ch);

int min(int lhs, int rhs);
int max(int lhs, int rhs);

char *strdup(char *src);
char *strndup(char *src, size_t n);
size_t strncount(const char *str, char ch, size_t n);
int stricmp(const char *lhs, const char *rhs);
int strnicmp(const char *lhs, const char *rhs, size_t n);
char *strfind(const char *str, char ch);
char *strrfind(const char *str, char ch);
char *strnfind(const char *str, char ch, size_t n);
char *strrnfind(const char *str, char ch, size_t n);
char *strfindl(const char *str, const char *chs, size_t n_ch);
char *strnfindl(const char *str, const char *chs, size_t n_ch, size_t n);
char *strfindnth(const char *str, char ch, size_t nth);
char *strnfindnth(const char *str, char ch, size_t nth, size_t n);

int file_exists(const char *path);
int open_file(const char *path, int flag, int perms);
size_t get_file_len(int fd);

void print_bits(const void *data, size_t bytes);
void print_buf_bits(const buf_t *buf);

char *find_space_n(const char *buf, size_t n);
char *skip_spaces_n(const char *buf, size_t n);

#endif /* UTILITY_H */
