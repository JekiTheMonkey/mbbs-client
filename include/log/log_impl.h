#ifndef LOG_IMPL_H
#define LOG_IMPL_H

#include "unused.h"

#ifdef _TRACE
    #define _RLOG_IMPL(...) fprintf(stderr, __VA_ARGS__)
#else
    #define _RLOG_IMPL(...)             ALL_UNUSED(__VA_ARGS__)
#endif

#endif /* LOG_IMPL_H */
