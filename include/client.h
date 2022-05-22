#ifndef CLIENT_H
#define CLIENT_H

#include "def.h"

#include <arpa/inet.h>

/* sst - Session STatus */
enum com_state {
    sst_unk,                            /* unknown */
    sst_disc,                           /* disconnect */
    sst_err,                            /* error */

    /* Read states */
    sst_lsn_rep,                        /* reply */
    sst_intro,                          /* wait for intro */
    sst_dnl,                            /* downloading file */

    /* Write states */
    sst_upl,                            /* uploading file */

    /* Read/write states */
    sst_idle,                           /* idle */
};

struct client
{
    com_state state;
    struct sockaddr_in serv_addr;
    int sfd;
    buffer *buf;
    size_t written_bytes;
    int demand_silence;
};

void client_init(client *cli, char **argv);
int client_start(client *cli);

#endif /* CLIENT_H */
