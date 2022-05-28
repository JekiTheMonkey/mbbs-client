#ifndef LOG_H
#define LOG_H

#include <assert.h>

#ifdef LOG_LVL
    #if LOG_LVL <= 1
        #define _TRACE
    #endif
#endif

/*
 * Some logging is enabled only with determined LOG_LVL
 */

#include "log_impl.h"
#include "log_mem.h"
#include "log_util.h"

/* E and L in log macros stand for 'Enter' and 'Leave' respectively */

#ifdef _TRACE
    /* Raw trace logging macro, no file information is provided */
    #define RLOG(...) do { _LOG_PRINT_TABS(); _LOG_IMPL(__VA_ARGS__); } while (0)

    /* Regular trace logging macro */
    #define _LOG_IMPL(SIGN, ...) \
        do { \
            _LOG_PRINT_TABS(); \
            _LOG_PRINT_INFO(SIGN); \
            _RLOG_IMPL(__VA_ARGS__); \
        } while (0)

    #define LOG(...) _LOG_IMPL(_LOG_STAY_SIGN, __VA_ARGS__)
    #define LOG_E(...) \
        do { \
            _LOG_IMPL(_LOG_IN_SIGN, __VA_ARGS__); \
            _LOG_INC_TABS(); \
        } while (0)
    #define LOG_L(...) \
        do { \
            _LOG_DEC_TABS(); \
            _LOG_IMPL(_LOG_OUT_SIGN, __VA_ARGS__); \
        } while (0)
    #define LOG_EX(...) \
        do { \
            _LOG_IMPL(_LOG_STAY_SIGN, __VA_ARGS__); \
            exit(1); \
        } while (0)

    /* Log a clear recursive function enterance event */
    #define LOG_EC(...) \
        do { \
            _LOG_PLOG_PRINT_INFO(_LOG_IN_SIGN); \
            _LOG_IMPL(__VA_ARGS__); \
        } while (0)
    #define LOG_LC(...) \
        do { \
            _LOG_PRINT_TABS(); \
            _LOG_PLOG_PRINT_INFO(_LOG_OUT_SIGN); \
            _LOG_IMPL(__VA_ARGS__); \
        } while (0)


    /* Trace logging macro with PID */
    #define _PLOG_IMPL(sign, ...) \
        do { \
            _LOG_PRINT_TABS(); \
            _LOG_PRINT_PINFO(sign); \
            _RLOG_IMPL(__VA_ARGS__); \
        } while (0)

    /* Process id LOG */
    #define PLOG(...) _PLOG_IMPL(_LOG_STAY_SIGN, __VA_ARGS__)
    #define PLOG_E(...) \
        do { \
            _PLOG_IMPL(_LOG_IN_SIGN, __VA_ARGS__); \
            _LOG_INC_TABS(); \
        } while (0)
    #define PLOG_L(...) \
        do { \
            _LOG_DEC_TABS(); \
            _PLOG_IMPL(_LOG_OUT_SIGN, __VA_ARGS__); \
        } while (0)

    /* Log a formatted string with a stdlib's error message afterwards */
    #define _ELOG_IMPL(SIGN, ...) \
        do { \
            _LOG_PRINT_TABS(); \
            _LOG_PRINT_INFO(SIGN); \
            _RLOG_IMPL(__VA_ARGS__); \
            _RLOG_IMPL(": %s\n", strerror(errno)); \
        } while (0)

    #define ELOG(...) _ELOG_IMPL(_LOG_STAY_SIGN, __VA_ARGS__)
    #define ELOG_E(...) \
        do { \
            _ELOG_IMPL(_LOG_IN_SIGN, __VA_ARGS__); \
            _LOG_INC_TABS(); \
        } while (0)
    #define ELOG_L(...) \
        do { \
            _LOG_DEC_TABS(); \
            _ELOG_IMPL(_LOG_OUT_SIGN, __VA_ARGS__); \
        } while (0)
    #define ELOG_EX(...) do { ELOG(__VA_ARGS__); exit(1); } while(0)

    /* Log a string with stdlib's error message afterwards */
    #define PELOG(msg) ELOG(msg)
    #define PELOG_E(msg) ELOG_E(msg)
    #define PELOG_L(msg) ELOG_L(msg)
    #define PELOG_EX(msg) do { ELOG(msg); exit(1); } while(0)


    /* Control and log ptr passed to a clear recursive function */
    #define LOG_CFNPP(ptr) \
        do { \
            assert(ptr); \
            LOG_EC(#ptr " addr %p *" #ptr " addr %p\n",  \
                (void *) (ptr), (void *) *(ptr)); \
        } while (0)
    /* Control and log ptr to ptr passed to a delete function */
    #define LOG_FNPP(ptr) \
        do { \
            assert(ptr); \
            assert(*ptr); \
            LOG_E(#ptr " addr %p *" #ptr " addr %p\n",  \
                (void *) (ptr), (void *) *(ptr)); \
        } while (0)
    /* Control and log ptr to ptr and item passed to an erase function */
    #define LOG_FNE(ptr, item) \
        do { \
            assert(ptr); \
            assert(*ptr); \
            assert(item); \
            LOG_E(#ptr " addr %p *" #ptr " addr %p " #item " addr %p\n", \
                (void *) (ptr), (void *) *(ptr), (void *) (item)); \
        } while (0)
#else
    #define RLOG(...)                   ALL_UNUSED(__VA_ARGS__)
    #define LOG(...)                    ALL_UNUSED(__VA_ARGS__)
    #define LOG_E(...)                  ALL_UNUSED(__VA_ARGS__)
    #define LOG_L(...)                  ALL_UNUSED(__VA_ARGS__)
    #define LOG_EX(...)                 ALL_UNUSED(__VA_ARGS__)
    #define LOG_EC(...)                 ALL_UNUSED(__VA_ARGS__)
    #define LOG_LC(...)                 ALL_UNUSED(__VA_ARGS__)
    #define PLOG(...)                   ALL_UNUSED(__VA_ARGS__)
    #define PLOG_E(...)                 ALL_UNUSED(__VA_ARGS__)
    #define PLOG_L(...)                 ALL_UNUSED(__VA_ARGS__)
    #define ELOG(...)                   ALL_UNUSED(__VA_ARGS__)
    #define ELOG_E(...)                 ALL_UNUSED(__VA_ARGS__)
    #define ELOG_L(...)                 ALL_UNUSED(__VA_ARGS__)
    #define ELOG_EX(...)                ALL_UNUSED(__VA_ARGS__)
    #define PELOG(msg)         perror(msg)
    #define PELOG_E(msg)       PELOG(msg)
    #define PELOG_L(msg)       PELOG(msg)
    #define PELOG_EX(msg) do { perror(msg); exit(1); } while(0)

    #define LOG_CFNPP(ptr)     do { assert(ptr); } while (0)
    #define LOG_FNPP(ptr)      do { assert(ptr); assert(*ptr); } while (0)
    #define LOG_FNE(ptr, item) do { LOG_FNPP(ptr); assert(item); } while(0)
#endif

#endif /* !LOG_H */
