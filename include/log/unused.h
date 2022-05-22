#ifndef LOG_UNUSED_H
#define LOG_UNUSED_H

#define UNUSED0()
#define UNUSED1(a)                  (void) (a)
#define UNUSED2(a,b)                (void) (a), UNUSED1(b)
#define UNUSED3(a,b,c)              (void) (a), UNUSED2(b,c)
#define UNUSED4(a,b,c,d)            (void) (a), UNUSED3(b,c,d)
#define UNUSED5(a,b,c,d,e)          (void) (a), UNUSED4(b,c,d,e)
#define UNUSED6(a,b,c,d,e,f)        (void) (a), UNUSED5(b,c,d,e,f)

#define VA_NUM_ARGS_IMPL(_0, _1, _2, _3, _4, _5, _6, N, ...) N
#define VA_NUM_ARGS(...) \
    VA_NUM_ARGS_IMPL(100, ## __VA_ARGS__, 6, 5, 4, 3, 2, 1, 0)

#define ALL_UNUSED_IMPL_(nargs) UNUSED ## nargs
#define ALL_UNUSED_IMPL(nargs) ALL_UNUSED_IMPL_(nargs)
#define ALL_UNUSED(...) \
    ALL_UNUSED_IMPL(VA_NUM_ARGS(__VA_ARGS__)) (__VA_ARGS__)

#endif /* LOG_UNUSED_H */
