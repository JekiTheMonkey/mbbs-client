#ifndef DEF_H
#define DEF_H

#define INV_MSG "mbbs> "
#define AUTH_KEY "mbbs-client"
#define DEF_DOW_DIR "mbbs-client-downloads"

#ifndef BUF_SIZE
#define BUF_SIZE 4096
#endif

#ifndef MAX_WRITE_BYTES
#define MAX_WRITE_BYTES 4096
#endif

typedef struct buf_t buf_t;
typedef struct cli_t cli_t;
typedef enum com_state com_state;
typedef enum cli_flags cli_flags;

#endif /* DEF_H*/
