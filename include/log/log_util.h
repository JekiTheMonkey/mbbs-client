#ifndef LOG_UTIL_H
#define LOG_UTIL_H

#define _TAB "    "
extern int tabs, _log_local_n;

#define EXEC_INFO __FILE__, __LINE__, __FUNCTION__
#define EXEC_PINFO EXEC_INFO, getpid()

#define _LOG_PRINT_TABS() \
    for (_log_local_n = 0; _log_local_n < tabs; _log_local_n++) _RLOG_IMPL(_TAB)
#define _LOG_PRINT_INFO(sign) \
    _RLOG_IMPL("%s: %d " sign " %s() ", EXEC_INFO)
#define _LOG__LOG_PRINT_PINFO(sign) \
    _RLOG_IMPL("%s: %d " sign " %s() [PID: %d] ", EXEC_PINFO)

/* Entrance/leave/stay signs */
#define _LOG_IN_SIGN "-->"
#define _LOG_OUT_SIGN "<--"
#define _LOG_STAY_SIGN "|"

#define _LOG_INC_TABS() do {                   tabs++; } while(0)
#define _LOG_DEC_TABS() do { assert(tabs > 0); tabs--; } while(0)

#endif /* LOG_UTIL_H */
