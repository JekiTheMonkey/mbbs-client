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
    sst_lsn_download,                   /* downloading file */

    /* Write states */
    sst_upload,                         /* uploading file */

    /* Read/write states */
    sst_idle,                           /* idle */
};

/* cfl - Client FLag */
enum cli_flags {
    cfl_demand_silence      = 1 << 0,   /* don't write invite message */
    cfl_download_overwrite  = 1 << 1,   /* wait for permission to overwrite a file */
    cfl_write_server        = 1 << 2,   /* send io_buffer contents to server */
    cfl_handle_input        = 1 << 3,   /* analyze and handle stdin input */
};

#define FLAGS_T int8_t

struct cli_t
{
    com_state state;
    FLAGS_T flags;
    struct sockaddr_in serv_addr;

    int sfd;
    int udfd;

    buf_t *io_buf;
    buf_t *ud_file_buf;
    buf_t *req_file_buf;
    char *dow_dir_path;

    size_t written_bytes;
    size_t read_bytes;
    size_t bytes_to_read;
};

void client_init(cli_t *cli, char **argv);
int client_start(cli_t *cli);

#endif /* CLIENT_H */
