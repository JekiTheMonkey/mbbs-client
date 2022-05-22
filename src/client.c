#include "buffer.h"
#include "client.h"
#include "log.h"
#include "utility.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#define DMD_SIL "DMD_SIL" /* server has demanded silence - no invite msg */

#define LOG_RET(code) do { LOG_L("\n"); return code; } while(0)
int create_socket()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
        ELOG_EX("Failed to create a socket");
    LOG("Socket(%d) has been created\n", fd);
    return fd;
}

void init_addr(struct sockaddr_in *addr, char **argv)
{
    const char *ip = argv[1];
    const short port = atoi(argv[2]);
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    addr->sin_addr.s_addr = inet_addr(ip);
    LOG("Initialized server address '%s::%d'\n", ip, port);
}

int was_signaled(int code)
{
    return code == -1 && errno == EINTR;
}

int check_err(int code)
{
    return code == -1 && errno != EINTR; /* return 1 on error */
}

int client_send_data(client *cli, const void *data, size_t bytes)
{
    int res;
    do {
        res = write(cli->sfd, data, min(bytes, MAX_WRITE_BYTES));
    } while (was_signaled(res));
    LOG("'%d' bytes have been written\n", res);

    if (res == -1)
        PELOG("Failed to read");
    else if (res == 0)
        LOG("EOF has been read\n");
    if (res <= 0)
    {
        cli->state= res ? sst_err : sst_disc;
        return -1;
    }

    cli->written_bytes += res;
    if (res < MAX_WRITE_BYTES)
        cli->written_bytes = 0;
    return res;
}

int client_send_str(client *cli, const char *str)
{
    return client_send_data(cli, str, strlen(str));
}

void client_connect(client *cli)
{
    if (connect(cli->sfd, (struct sockaddr *) &cli->serv_addr,
        sizeof(cli->serv_addr)))
        ELOG_EX("Failed to connect to server");
    cli->state = sst_intro;
    client_send_str(cli, AUTH_KEY);
    LOG("Connection has been established\n");
}

void client_init(client *cli, char **argv)
{
    LOG_E("\n");
    cli->sfd = create_socket();
    init_addr(&cli->serv_addr, argv);
    cli->buf = buffer_create(BUF_SIZE);
    cli->state = sst_unk;
    cli->demand_silence = 1;
    LOG_L("Client has been initialized\n");
}

void handle_invite_msg(client *cli)
{
    if (!cli->demand_silence)
        write(0, INV_MSG, sizeof(INV_MSG));
    cli->demand_silence = 0;
}

int init_fds(client *cli, fd_set *readfds, fd_set *writefds)
{
    FD_ZERO(readfds);
    FD_ZERO(writefds);
    FD_SET(cli->sfd, readfds);
    FD_SET(0, readfds); /* stdin */
    return cli->sfd;
}

int mselect(client *cli, fd_set *readfds, fd_set *writefds)
{
    int maxfd = init_fds(cli, readfds, writefds);
    return select(maxfd + 1, readfds, writefds, NULL, NULL);
}

int client_receive_data(client *cli, int fd)
{
    int res;
    buffer *buf = cli->buf;
    do {
        res = read(fd, buf->ptr + buf->used, buf->size - buf->used);
    } while (was_signaled(res));

    if (res == -1)
        ELOG("Failed to read");
    else if (res == 0)
        LOG("EOF has been read\n");
    if (res <= 0)
    {
        cli->state= res ? sst_err : sst_disc;
        return -1;
    }

    buf->used += res;
    LOG("'%d' bytes have been read\n", res);
    return res;
}

int control_buffer_fullness(client *cli)
{
    const buffer *buf = cli->buf;
    if (buf->used != buf->size)
        return 0;
    LOG("Input line is too long, terminate connection with a client\n");
    LOG("Your input line is too long, bye...\n");
    cli->state = sst_err;
    return 1;
}

int handle_stdin_read(client *cli, fd_set *readfds)
{
    if (!FD_ISSET(0, readfds))
        return 0;

    LOG_E("\n");
    buffer *buf = cli->buf;
    assert(buf->used == 0);
    if (client_receive_data(cli, 0) == -1)
        return -1;
    if (control_buffer_fullness(cli))
        LOG_RET(-1);
    client_send_data(cli, buf->ptr, buf->used);
    buffer_clear(buf);
    cli->state = sst_lsn_rep;
    cli->demand_silence = 1;
    LOG_L("\n");
    return 1;
}

int handle_lsn_intro(client *cli)
{
    if (cli->state != sst_intro)
        return 0;
    buffer *buf = cli->buf;
    write(1, buf->ptr, buf->used);
    buffer_clear(buf);
    cli->state = sst_idle;
    return 1;
}

int handle_lsn_rep(client *cli)
{
    if (cli->state != sst_lsn_rep)
        return 0;
    buffer *buf = cli->buf;
    if (!memcmp(buf->ptr, DMD_SIL, sizeof(DMD_SIL) - 1))
    {
        buffer_move_left(buf, sizeof(DMD_SIL) - 1); /* remove given demand */
        cli->demand_silence = 1;
    }
    write(1, buf->ptr, buf->used);
    buffer_clear(buf);
    cli->state = sst_idle;
    return 1;
}

int handle_lsn_idle(client *cli)
{
    if (cli->state != sst_idle)
        return 0;
    buffer *buf = cli->buf;
    write(1, buf->ptr, buf->used);
    buffer_clear(buf);
    return 1;
}

#define _HDL_RES(code) \
    do { \
        if (code == -1) \
            LOG_RET(-1); /* disconnect */ \
        else if (code > 0) \
            LOG_RET(1); \
    } while (0)

#define HDL_LSN(lsn_to, cli) \
    do { \
        res = handle_lsn_ ##lsn_to (cli); \
        _HDL_RES(res); \
    } while(0)

int handle_states(client *cli)
{
    int res;
    /* TOFIX (maybe I just do something wrong) */
    /* for some reason macro processor can't implictly take AND REMEMBER
       an anonym variable, so return value is saves into 'res' variable instead */
    LOG_E("\n");
    HDL_LSN(intro, cli);
    HDL_LSN(rep, cli);
    HDL_LSN(idle, cli);
    /* handle_download */
    LOG("An unhandled communication state has been found\n");
    exit(1);
}

int handle_server_read(client *cli, fd_set *readfds)
{
    if (!FD_ISSET(cli->sfd, readfds))
        return 0;
    LOG_E("\n");

    if (client_receive_data(cli, cli->sfd) == -1)
        LOG_RET(-1);
    handle_states(cli);
    if (control_buffer_fullness(cli))
        LOG_RET(-1);

    LOG_L("\n");
    return 1;
}

int handle_server_write(client *cli, fd_set *writefds)
{
    if (!FD_ISSET(cli->sfd, writefds))
        return 0;

    LOG_E("\n");
    /* handle file uploading */
    LOG_L("\n");
    return 1;
}

void check_io(client *cli, fd_set *readfds, fd_set *writefds)
{
    /* -1 = error | 0 = didn't do anything | 1 = done */
    if (handle_stdin_read(cli, readfds) ||
        handle_server_read(cli, readfds) ||
        handle_server_write(cli, writefds))
        return;
}

int handle_events(int code, client *cli, fd_set *readfds, fd_set *writefds)
{
    LOG_E("\n");
    if (was_signaled(code))
        LOG_RET(1);
    else if (check_err(code))
        LOG_RET(-1);
    check_io(cli, readfds, writefds);
    LOG_L("\n");
    /* -1 = error | 0 = successful disconnect | 1 = handled event */
    return cli->state == sst_disc ? 0 : cli->state == sst_err ? -1 : 1;
}

int client_start(client *cli)
{
    LOG_E("\n");
    fd_set readfds, writefds;
    client_connect(cli);
    for (;;) /* Main loop */
    {
        handle_invite_msg(cli);

        /* Select events */
        int res = mselect(cli, &readfds, &writefds);
        LOG("Select has returned '%d'\n", res);

        /* Handle events */
        res = handle_events(res, cli, &readfds, &writefds);
        if (res <= 0)
            LOG_RET(res == -1); /* return 1 on error, 0 on success */
    }
    printf("\nTerminating own work...\n");
    close(cli->sfd);
    LOG_L("\n");
}
