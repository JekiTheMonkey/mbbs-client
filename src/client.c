#define _XOPEN_SOURCE 500 /* snprintf */
#include "buffer.h"
#include "client.h"
#include "flags.h"
#include "log.h"
#include "utility.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <unistd.h>

/* ######################### Client related strings ######################### */
/* Service commands */
#define CMD_DOW  "download"
#define CMD_UPL  "upload"

/* Messages for communication */
#define TERM_MSG "Terminating own work..."
#define TERM_DOW "File has been successfully downloaded"
#define TERM_UPL "File has been successfully uploaded"

#define LIN_2LG  "Your input line is too long. Try a shorter command..."
#define F_EX     "Warning: File '%s' already exists, do you want to " \
    "overwrite it? [Y/N]: "
#define IMP_OP   "Fatal error: You are trying to "

/* ##################### Client & Server related strings #################### */
/* --- System commands --- */
/* Server messages */
#define DMD_SIL "DMD_SIL" /* Server has demanded silence - no invite msg */
#define DOW_DET "DOW_DET" /* Download file's details */
#define UPL_ALW "UPL_ALW" /* Server has allowed to upload a file */
/* Client messages */
#define DOW_ACC "DOW_ACC" /* Accept to download a file */

/* ########################################################################## */

#define CMDMEMCMP(ptr, cmd) (memcmp(ptr, cmd, sizeof(cmd) - 1))
#define LOG_RET(code) do { LOG_L("\n"); return code; } while(0)
#define CLIENT_SEND_CMD(cli, cmd) client_send_data(cli, cmd, sizeof(cmd ) - 1)

int create_socket()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
        PELOG_EX("Failed to create a socket");
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

int check_err(int code)
{
    return code == -1 && errno != EINTR; /* return 1 on error */
}

int client_send_data(cli_t *cli, const void *data, size_t bytes)
{
    assert(bytes != 0);
    int res;
    do {
        res = write(cli->sfd, data, min(bytes, MAX_WRITE_BYTES));
    } while (was_signaled(res));
    LOG("'%d' bytes have been written\n", res);

    if (res == -1)
    {
        ELOG("Failed to write");
        cli->state = sst_err;
        return -1;
    }
    return res;
}

int client_send_str(cli_t *cli, const char *str)
{
    return client_send_data(cli, str, strlen(str));
}

void client_connect(cli_t *cli)
{
    if (connect(cli->sfd, (struct sockaddr *) &cli->serv_addr,
        sizeof(cli->serv_addr)))
        PELOG_EX("Failed to connect to server");
    LOG("Change state to %d\n", sst_intro);
    cli->state = sst_intro;
    CLIENT_SEND_CMD(cli, AUTH_KEY);
    LOG("Connection has been established\n");
}

void create_dir_if_not_exists(const char *path)
{
    if (access(path, F_OK))
    {
        LOG("Missing directory '%s'\n", path);
        if (mkdir(path, 0770))
            ELOG_EX("Failed to create directory '%s'", path);
        LOG("Created directory '%s'\n", path);
    }
}

void init_dow_dir_path(cli_t *cli)
{
    /*
     * TODO Create a config that will allow to choose downloads destination
     */
    cli->dow_dir_path = (char *) malloc(sizeof(DEF_DOW_DIR));
    strcpy(cli->dow_dir_path, DEF_DOW_DIR);
}

void client_init(cli_t *cli, char **argv)
{
    LOG_E("\n");
    cli->sfd = create_socket();
    cli->udfd = 0;
    init_addr(&cli->serv_addr, argv);
    cli->io_buf = buffer_create(BUF_SIZE);
    cli->ud_file_buf = buffer_create(BUF_SIZE);
    cli->req_file_buf = buffer_create(BUF_SIZE);
    cli->state = sst_unk;
    cli->flags = cfl_demand_silence;
    cli->bytes_to_write = 0;
    cli->read_bytes = 0;
    cli->bytes_to_read = 0;

    create_dir_if_not_exists(DEF_DOW_DIR);
    init_dow_dir_path(cli);
    LOG_L("Client has been initialized\n");
}

/* ========================================================================== */
/* ========================================================================== */
/* ========================================================================== */

void handle_invite_msg(cli_t *cli)
{
    if (!(cli->flags & cfl_demand_silence))
        write(0, INV_MSG, sizeof(INV_MSG));
    FLG_RM(cli->flags, cfl_demand_silence);
}

int init_fds(cli_t *cli, fd_set *readfds, fd_set *writefds)
{
    FD_ZERO(readfds);
    FD_ZERO(writefds);
    FD_SET(cli->sfd, readfds);
    FD_SET(0, readfds); /* stdin */
    if (cli->state == sst_upload)
        FD_SET(cli->sfd, writefds);
    return cli->sfd;
}

int mselect(cli_t *cli, fd_set *readfds, fd_set *writefds)
{
    int maxfd = init_fds(cli, readfds, writefds);
    return select(maxfd + 1, readfds, writefds, NULL, NULL);
}

int client_receive_data(cli_t *cli, int fd)
{
    int res;
    buf_t *buf = cli->io_buf;
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

int control_buffer_fullness(cli_t *cli)
{
    buf_t *buf = cli->io_buf;
    if (buf->used != buf->size)
        return 0;
    write(1, LIN_2LG "\n", sizeof(LIN_2LG) + 1);
    buffer_clear(buf);
    return 1;
}

int open_download_file(cli_t *cli)
{
    const char *file = (char *) cli->ud_file_buf->ptr;
    int fd = open_file(file, O_WRONLY | O_CREAT | O_TRUNC, 0770);
    LOG("Opened file '%s' (%d)\n", file, fd);
    if (fd == -1)
    {
        PELOG("Failed to create a file for download");
        return -1;
    }
    return fd;
}

void download_prepare_request(cli_t *cli)
{
    buf_t *src = cli->req_file_buf, *dest = cli->io_buf;
    assert(src->used > 0);
    buffer_clear(dest);
    buffer_appendf(dest, "%.*s %.*s",
        (int) sizeof(CMD_DOW) - 1, CMD_DOW, (int) src->used, src->ptr);
    LOG("Request: '%s' (%lu)\n", (char *) dest->ptr, dest->used);
}

int check_flags(cli_t *cli)
{
    int res = 0;
    FLAGS_T *flags = &cli->flags;
    buf_t *io_buf = cli->io_buf;
    const char *ch_buf = (char *) io_buf->ptr;
    const unsigned len = io_buf->used;
    if (!*flags)
        return 0;

    LOG_E("Bits:\n");
    FLG_PRINT(*flags);
    if (*flags & cfl_download_overwrite)
    {
        LOG("Download overwrite reply analyze\n");
        if (len == 0 || (len == 1 && tolower(ch_buf[0]) == 'y'))
        {
            LOG("Yes\n");
            buffer_clear(io_buf);
            download_prepare_request(cli);
        }
        else
        {
            LOG("No\n");
            FLG_RM(*flags, cfl_write_server);
            buffer_clear(cli->ud_file_buf);
            buffer_clear(cli->req_file_buf);
            res++;
        }
        FLG_RM(*flags, cfl_download_overwrite | cfl_handle_input);
        res++;
    }
    LOG_L("\n");
    return res;
}

int handle_empty(cli_t *cli)
{
    const buf_t *buf = cli->io_buf;
    if (!buf->used)
        FLG_RM(cli->flags, cfl_write_server);
    return !buf->used;
}

/* int handle_upload_req(client *cli) */
/* { */
/*     if (memcmp(cli->io_buf->ptr, CMD_UPL, sizeof(CMD_UPL) - 1)) */
/*         return 0; */
/*     buf_t *buf = cli->io_buf; */
/*     const char *path = (char *) buf->ptr + sizeof(CMD_UPL); */
/*     int fd = file_exists(path); */
/*     if (fd == -1) */
/*     { */
/*         /1* TODO ask if overwrite *1/ */
/*         buffer_clear(buf); */
/*         return -1; */
/*     } */

/*     const size_t bytes = get_file_len(fd); */
/*     char ch_buf[32]; */
/*     int res = sprintf(ch_buf, "%lu", bytes); */
/*     res--; /1* don't count \0 *1/ */
/*     if (buf->used + res + 1 > buf->size) */
/*     { */
/*         write(1, LIN_2LG "\n", sizeof(LIN_2LG) + 1); */
/*         buffer_clear(buf); */
/*         return -1; */
/*     } */

/*     void *base_ptr = buf->ptr + sizeof(CMD_UPL); */
/*     memmove(base_ptr + res, base_ptr, buf->used - sizeof(CMD_UPL)); */
/*     memcpy(base_ptr, ch_buf, res); */
/*     buf->used += res; */
/*     write(1, buf->ptr, buf->used); */
/*     exit(1); */

/*     close(fd); */
/*     return 1; */
/* } */

/*
 * argv1 = command
 * argv2 = file name on server
 * argv3 (opt) = filepath on client's computer
 */
int copy_filepath(cli_t *cli)
{
    char *arg, *dow_dir = cli->dow_dir_path;
    buf_t *buf = cli->io_buf;
    buf_t *filepath = cli->ud_file_buf;

    assert(filepath->used == 0);
    const unsigned argc = buffer_count_argc(buf);
    LOG_E("Arguments: %d\n", argc);
    if (argc == 1 || argc > 3)
        LOG_RET(0);
    if (argc == 2) /* save in download directory with the same name */
    {
        arg = buffer_get_argv_n(buf, 2);
        buffer_appendf(filepath, "%s/", dow_dir);
        LOG("First step: '%s'\n", (char *) filepath->ptr);
    }
    else /* argc == 3. save in a given path */
        arg = buffer_get_argv_n(buf, 3);

    assert(arg);
    const unsigned skip_chs = (void *) arg - buf->ptr;
    buffer_appendf(
        filepath, "%.*s", buf->used - skip_chs, (char *) buf->ptr + skip_chs);

    LOG_L("Saved filepath %d '%s'\n",
        (int) filepath->used, (char *) filepath->ptr);
    return 1;
}

int check_download_file_existance(cli_t *cli)
{
    const buf_t *buf = cli->ud_file_buf;
    const char *filepath = (char *) buf->ptr;
    assert(buf->used > 0);
    LOG("Check if '%s' exists\n", filepath);
    if (file_exists(filepath))
    {
        printf(F_EX, filepath);
        fflush(stdout);
        FLG_ADD(cli->flags, cfl_download_overwrite | cfl_demand_silence);
        FLG_RM(cli->flags, cfl_write_server);
        return 1;
    }
    return 0;
}

void save_requested_file(cli_t *cli)
{
    buf_t *src = cli->io_buf, *dest = cli->req_file_buf;
    const unsigned argc = buffer_count_argc(src);
    const char *begin, *end;
    begin = buffer_get_argv_n(src, 2);
    end = argc != 3 ? (char *) src->ptr + src->used :
        find_space_n(begin, src->used - ((void *) begin - src->ptr));
    assert(begin);
    assert(end);
    if (dest->used != 0)
        LOG("dest->used '%lu'\n", dest->used);
    buffer_clear(dest);
    buffer_append(dest, begin, end - begin);
}

int try_create_download_file(cli_t *cli)
{
    const buf_t *buf = cli->ud_file_buf;
    const char *filepath = (char *) buf->ptr;
    assert(buf->used > 0);
    LOG("Trying to create '%s'\n", filepath);
    int fd = open_download_file(cli);
    if (fd == -1)
        return 0;
    LOG("Close FD '%d'\n", fd);
    close(fd);
    return 1;
}

int check_download_request(cli_t *cli)
{
    buf_t *buf = cli->io_buf;
    if (CMDMEMCMP(buf->ptr, CMD_DOW))
        return 0;

    LOG_E("\n");
    save_requested_file(cli);
    if (!copy_filepath(cli))
        LOG_RET(1);
    if (check_download_file_existance(cli))
        LOG_RET(-1);
    if (!try_create_download_file(cli))
    {
        FLG_RM(cli->flags, cfl_write_server);
        buffer_clear(cli->ud_file_buf);
        buffer_clear(cli->req_file_buf);
        LOG_RET(-1);
    }
    download_prepare_request(cli);
    FLG_ADD(cli->flags, cfl_write_server | cfl_demand_silence);
    LOG_L("\n");
    return 1;
}

void copy_upload_filename(buf_t *dest, const buf_t *src)
{
    const char delims[] = { ' ', '\t', '\0' };
    const char *filename = buffer_get_argv_n((buf_t *) src, 2);
    assert(filename);
    const unsigned offset = (void *) filename - src->ptr;
    const char *end =
        strnfindl(filename, delims, sizeof(delims), src->used - offset);
    assert(end);
    const int len = end - filename;

    LOG("To copy(%d): '%.*s'\n", len, len, filename);
    memmove(dest->ptr, filename, len);
    memset(dest->ptr + len, 0, 1);
    dest->used = len + 1;
}

int check_upload_request(cli_t *cli)
{
    buf_t *buf = cli->io_buf;
    if (CMDMEMCMP(buf->ptr, CMD_UPL))
        return 0;

    LOG_E("\n");
    char *source = buffer_get_argv_n(buf, 2);
    if (!source)
        LOG_RET(1);
    char *file = buffer_get_argv_n(buf, 3);
    if (!file) /* upload file with original name */
        file = source;
    buffer_append(buf, "", 1);
    buffer_clear(cli->ud_file_buf);
    copy_upload_filename(cli->ud_file_buf, buf);

    int fd = open_file((char *) cli->ud_file_buf->ptr, O_RDONLY, 0);
    if (fd == -1)
    {
        FLG_RM(cli->flags, cfl_write_server);
        LOG_RET(-1);
    }
    const size_t size = get_file_len(fd);
    buf->used =
        snprintf((char *) buf->ptr, buf->size, CMD_UPL " %s %lu", file, size);
    LOG("Formed message: '%.*s'\n", (int) buf->used, (char *) buf->ptr);
    cli->bytes_to_write = size;

    close(fd);
    LOG_L("\n");
    return 1;
}

int handle_req_idle(cli_t *cli)
{
    if (cli->state != sst_idle)
        return 0;
    if (check_download_request(cli) ||
        check_upload_request(cli))
        return 1;
    LOG("Change state to %d\n", sst_lsn_rep);
    cli->state = sst_lsn_rep;
    return 1;
}

int handle_stdin_input(cli_t *cli)
{
    if (handle_empty(cli) ||
        handle_req_idle(cli))
        return 1;

    LOG_EX("An unhandled state has been catched.\n");
    return 0;
}

void remove_lf(buf_t *buf)
{
    if (*((char *) buf->ptr + buf->used - 1) == '\n')
        buffer_move_right(buf, 1);
}

int read_stdin_input(cli_t *cli)
{
    buf_t *buf = cli->io_buf;
    assert(buf->used == 0);
    if (client_receive_data(cli, 0) == -1)
        return -1; /* error */
    if (control_buffer_fullness(cli))
        return -2; /* drop io buffer */
    remove_lf(buf);
    LOG("Input: '%.*s'\n", (int) buf->used, (char *) buf->ptr);
    return 1; /* success */
}

int handle_stdin_read(cli_t *cli, fd_set *readfds)
{
    if (!FD_ISSET(0, readfds))
        return 0;

    LOG_E("\n");
    const com_state state = cli->state;
    buf_t *buf = cli->io_buf;
    int res = read_stdin_input(cli);
    if (res < 0)
        LOG_RET(res == -2 ? 1 : -1);

    /* Handle requests and write input to server by default */
    FLG_ADD(cli->flags, cfl_write_server | cfl_handle_input);

    if (check_flags(cli))
    {
        LOG("Flags:\n");
        FLG_PRINT(cli->flags);
    }
    if (cli->flags & cfl_handle_input)
    {
        handle_stdin_input(cli);
        FLG_RM(cli->flags, cfl_handle_input);
        LOG("Flags:\n");
        FLG_PRINT(cli->flags);
    }
    if (cli->flags & cfl_write_server)
    {
        client_send_data(cli, buf->ptr, buf->used);
        FLG_ADD(cli->flags, cfl_demand_silence);
        FLG_RM(cli->flags, cfl_write_server);
        if (state == cli->state)
        {
            LOG("Change state to %d\n", sst_lsn_rep);
            cli->state = sst_lsn_rep;
        }
    }
    buffer_clear(buf);

    LOG_L("\n");
    return 1;
}

int handle_lsn_intro(cli_t *cli)
{
    if (cli->state != sst_intro)
        return 0;
    LOG("Use this handle...\n");
    buf_t *buf = cli->io_buf;
    write(1, buf->ptr, buf->used);
    buffer_clear(buf);
    LOG("Change state to %d\n", sst_idle);
    cli->state = sst_idle;
    return 1;
}

int handle_cmd_dmd_sil(cli_t *cli)
{
    buf_t *buf = cli->io_buf;
    buffer_move_left(buf, sizeof(DMD_SIL) - 1);
    FLG_ADD(cli->flags, cfl_demand_silence);
    return 1;
}

int handle_cmd_dow_det(cli_t *cli)
{
    buf_t *buf = cli->io_buf;
    buffer_move_left(buf, sizeof(DOW_DET) - 1);
    cli->bytes_to_read = atoi((char *) buf->ptr);
    LOG("Bytes to download: %lu\n", cli->bytes_to_read);
    /*
     * TODO Check if memory is enough to store the whole file
     */
    cli->udfd = open_download_file(cli);
    assert(cli->udfd != -1);
    FLG_ADD(cli->flags, cfl_demand_silence);
    LOG("Change state to %d\n", sst_lsn_download);
    cli->state = sst_lsn_download;
    buffer_clear(buf);
    buffer_clear(cli->ud_file_buf);
    buffer_clear(cli->req_file_buf);
    CLIENT_SEND_CMD(cli, DOW_ACC);
    return 2;
}

int handle_cmd_upl_alw(cli_t *cli)
{
    buf_t *buf = cli->io_buf;
    LOG("Opening '%s'...\n", (char *) cli->ud_file_buf->ptr);
    cli->udfd = open_file((char *) cli->ud_file_buf->ptr, O_RDONLY, 0);
    assert(cli->udfd != -1);
    FLG_ADD(cli->flags, cfl_demand_silence);
    LOG("Change state to %d\n", sst_upload);
    cli->state = sst_upload;
    buffer_clear(buf);
    buffer_clear(cli->ud_file_buf);
    buffer_clear(cli->req_file_buf);
    return 2;
}

int handle_special_replies(cli_t *cli)
{
    buf_t *buf = cli->io_buf;
    const char *cmd = (char *) buf->ptr;
    LOG_E("Command: '%.*s'\n", (int) buf->used, cmd);

    if (!CMDMEMCMP(cmd, DMD_SIL) && handle_cmd_dmd_sil(cli) > 2)
        LOG_RET(1);
    else if (!CMDMEMCMP(cmd, DOW_DET) && handle_cmd_dow_det(cli) > 2)
        LOG_RET(1);
    else if (!CMDMEMCMP(cmd, UPL_ALW) && handle_cmd_upl_alw(cli) > 2)
        LOG_RET(1);
    else
        LOG("No special reply has been found\n");
    LOG_L("\n");
    return 0;
}

int handle_lsn_reply(cli_t *cli)
{
    if (cli->state != sst_lsn_rep)
        return 0;
    LOG("Use this handle...\n");
    buf_t *buf = cli->io_buf;
    if (handle_special_replies(cli))
    {
        LOG("Return due some special reply\n");
        return 1;
    }
    write(1, buf->ptr, buf->used);
    buffer_clear(buf);
    LOG("Change state to %d\n", sst_idle);
    cli->state = sst_idle;
    return 1;
}

int handle_lsn_idle(cli_t *cli)
{
    if (cli->state != sst_idle)
        return 0;
    LOG("Use this handle...\n");
    buf_t *buf = cli->io_buf;
    write(1, buf->ptr, buf->used);
    buffer_clear(buf);
    return 1;
}

void terminate_download(cli_t *cli)
{
    write(1, TERM_DOW "\n", sizeof(TERM_DOW) + 1);
    cli->state = sst_idle;
    cli->read_bytes = 0;
    cli->bytes_to_read = 0;
    close(cli->udfd);
    LOG("Download file descriptor '%d' has been closed\n", cli->udfd);
    cli->udfd = 0;
}

int handle_lsn_download(cli_t *cli)
{
    if (cli->state != sst_lsn_download)
        return 0;
    LOG("Use this handle...\n");
    assert(cli->udfd > 2);
    buf_t *buf = cli->io_buf;
    assert(buf->used <= cli->bytes_to_read);
    int res = write(cli->udfd, buf->ptr, buf->used);
    if (res == -1)
    {
        PELOG("Failed to write to file");
        cli->state = sst_disc;
    }
    else
    {
        cli->read_bytes += res;
        if (cli->read_bytes == cli->bytes_to_read)
            terminate_download(cli);
        else
        {
            FLG_ADD(cli->flags, cfl_demand_silence);
            LOG("Remained: '%lu'\n", cli->bytes_to_read - cli->read_bytes);
        }
    }
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
#define HDL_WRITE(write_to, cli) \
    do { \
        res = handle_write_ ##write_to (cli); \
        _HDL_RES(res); \
    } while(0)

int handle_lsn_states(cli_t *cli)
{
    int res;
    LOG_E("State %d\n", cli->state);
    HDL_LSN(intro, cli);
    HDL_LSN(reply, cli);
    HDL_LSN(idle, cli);
    HDL_LSN(download, cli);
    LOG("An unhandled communication state has been found\n");
    exit(1);
}

int handle_server_read(cli_t *cli, fd_set *readfds)
{
    if (!FD_ISSET(cli->sfd, readfds))
        return 0;
    LOG_E("\n");
    buf_t *buf = cli->io_buf;

    if (client_receive_data(cli, cli->sfd) == -1)
        LOG_RET(-1);
    if (cli->state != sst_lsn_download)
        buffer_print_bin(buf);
    handle_lsn_states(cli);
    if (control_buffer_fullness(cli))
        LOG_RET(1);

    LOG_L("\n");
    return 1;
}

void terminate_upload(cli_t *cli)
{
    write(1, TERM_UPL "\n", sizeof(TERM_UPL) + 1);
    cli->state = sst_idle;
    cli->written_bytes = 0;
    cli->bytes_to_write = 0;
    close(cli->udfd);
    LOG("Upload file descriptor '%d' has been closed\n", cli->udfd);
    cli->udfd = 0;
    LOG("Change state to %d\n", sst_idle);
    cli->state = sst_idle;
    buffer_clear(cli->req_file_buf);
}

int handle_write_upload(cli_t *cli)
{
    if (cli->state != sst_upload)
        return 0;
    LOG("Use this handle...\n");
    assert(cli->udfd > 2);

    buf_t *buf = cli->io_buf;
    int res = read(cli->udfd, buf->ptr, buf->size);
    LOG("Read data - '%.*s'\n", res, (char *) buf->ptr);
    res = client_send_data(cli, buf->ptr, res);
    if (res == -1)
    {
        PELOG("Failed to read upload file");
        cli->state = sst_disc;
    }
    else
    {
        cli->written_bytes += res;
        if (cli->written_bytes == cli->bytes_to_write)
            terminate_upload(cli);
        else
        {
            FLG_ADD(cli->flags, cfl_demand_silence);
            LOG("Remained: '%lu'\n", cli->bytes_to_write - cli->written_bytes);
        }
    }

    return 1;
}

int handle_server_write(cli_t *cli, fd_set *writefds)
{
    if (!FD_ISSET(cli->sfd, writefds))
        return 0;

    int res;
    LOG_E("\n");
    HDL_WRITE(upload, cli);
    LOG_L("\n");
    return 1;
}

void check_io(cli_t *cli, fd_set *readfds, fd_set *writefds)
{
    /* -1 = error | 0 = didn't do anything | 1 = done */
    handle_stdin_read(cli, readfds);
    if (handle_server_read(cli, readfds) ||
        handle_server_write(cli, writefds))
        return;
}

int handle_events(int code, cli_t *cli, fd_set *readfds, fd_set *writefds)
{
    LOG_E("State %d\n", cli->state);
    if (was_signaled(code))
        LOG_RET(1);
    else if (check_err(code))
        LOG_RET(-1);
    check_io(cli, readfds, writefds);
    LOG_L("\n");
    /* -1 = error | 0 = successful disconnect | 1 = handled event */
    return cli->state == sst_disc ? 0 : cli->state == sst_err ? -1 : 1;
}

int client_start(cli_t *cli)
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
        LOG("\n\n");
    }
    write(1, "\n" TERM_MSG "\n", sizeof(TERM_MSG) + 2);
    close(cli->sfd);
    LOG_L("\n");
}
