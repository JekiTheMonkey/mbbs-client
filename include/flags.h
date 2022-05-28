#ifndef FLAGS_H
#define FLAGS_H

#define FLG_ADD(flags, to_add) (flags |= to_add)
#define FLG_RM(flags, to_rm) (flags &= ~(to_rm))
#ifdef _TRACE
    #define FLG_PRINT(flags) print_bits((void *) &flags, sizeof(flags))
#else
    #define FLG_PRINT(flags) UNUSED1(flags)
#endif

#endif /* FLAGS_H */
