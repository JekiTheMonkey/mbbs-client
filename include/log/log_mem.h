#ifndef LOG_MEM_H
#define LOG_MEM_H

#include "log_util.h"
#include "unused.h"

#ifdef _TRACE
    #define _ALLOC_LOG_MSG "Allocated %lu bytes for ptr '%p'\n"

    /* Log a memory free call */
    #define LOG_FREE(ptr) \
        do { \
            LOG("free '" #ptr "' %p\n", (void *) (ptr)); \
        } while (0)
    #define LOG_FREE_IFEX(ptr) \
        do { \
            LOG("free if exists '" #ptr "' %p\n", (void *) (ptr)); \
        } while (0)

    /* Log and perform a memory free call; set the ptr to NULL */
    #define _FREE_IMPL(ptr) \
        do { free(ptr); (ptr) = NULL; _FREED_IMPL(ptr); } while (0)
    #define _FREED_IMPL(ptr) \
        LOG(#ptr " has been freed and set to NULL\n");

    #define FREE(ptr) \
        do { LOG_FREE(ptr); _FREE_IMPL(ptr); } while (0)
    /* Log and perform a memory free call only IF EXists  */
    #define FREE_IFEX(ptr) \
        do { LOG_FREE_IFEX(ptr); if (ptr) _FREE_IMPL(ptr); } while (0)

    /* Allocation LOG */
    /* Log memory allocation with implicit allocated bytes amount */
    /* Note - won't work properly if bytes amount can't be deduced with sizeof(*ptr) */
    #define _ALOG_IMPL(sign, ptr) \
        do { \
            _LOG_PRINT_TABS(); \
            _LOG_PRINT_INFO(sign); \
            _RLOG_IMPL(_ALLOC_LOG_MSG, sizeof(*ptr), (void *) (ptr)); \
        } while (0)

    #define ALOG(ptr) _ALOG_IMPL(_LOG_STAY_SIGN, ptr)
    #define ALOG_E(ptr) \
        do { \
            _ALOG_IMPL(_LOG_IN_SIGN, ptr); \
            _LOG_INC_TABS(); \
        } while (0)
    #define ALOG_L(ptr) \
        do { \
            _LOG_DEC_TABS(); \
            _ALOG_IMPL(_LOG_OUT_SIGN, ptr); \
        } while (0)


    /* Allocation Number LOG */
    /* Log memory allocation with explicit allocated bytes amount */
    #define _ANLOG_IMPL(sign, ptr, n) \
        do { \
            _LOG_PRINT_TABS(); \
            _LOG_PRINT_INFO(sign); \
            _RLOG_IMPL(_ALLOC_LOG_MSG, (size_t) (n), (void *) (ptr)); \
        } while (0)

    #define ANLOG(ptr, n) _ANLOG_IMPL(_LOG_STAY_SIGN, ptr, n)
    #define ANLOG_E(ptr, n) \
        do { \
            _ANLOG_IMPL(_LOG_IN_SIGN, ptr, n); \
            _LOG_INC_TABS(); \
        } while (0)
    #define ANLOG_L(ptr, n) \
        do { \
            _LOG_DEC_TABS(); \
            _ANLOG_IMPL(_LOG_OUT_SIGN, ptr, n); \
        } while (0)
#else

    #define ALOG(...)                   ALL_UNUSED(__VA_ARGS__)
    #define ALOG_E(...)                 ALL_UNUSED(__VA_ARGS__)
    #define ALOG_L(...)                 ALL_UNUSED(__VA_ARGS__)
    #define ANLOG(...)                  ALL_UNUSED(__VA_ARGS__)
    #define ANLOG_E(...)                ALL_UNUSED(__VA_ARGS__)
    #define ANLOG_L(...)                ALL_UNUSED(__VA_ARGS__)

    #define FREE(ptr) free(ptr)
    #define FREE_IFEX(ptr) do { if (ptr) free(ptr); } while(0)
#endif

#endif /* LOG_MEM_H */
