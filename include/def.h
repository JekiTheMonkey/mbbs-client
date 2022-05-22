#ifndef DEF_H
#define DEF_H

#define INV_MSG "mbbs> "
#define AUTH_KEY "mbbs-client"

#ifndef BUF_SIZE
#define BUF_SIZE 4096
#endif

#ifndef MAX_WRITE_BYTES
#define MAX_WRITE_BYTES 4096
#endif

typedef struct buffer buffer;
typedef struct client client;
typedef enum com_state com_state;

#endif /* DEF_H*/
