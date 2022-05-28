#include "client.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Usage: ./mbbs-client <IP> <port>\n");
        return 0;
    }

    cli_t cli;
    client_init(&cli, argv);
    return client_start(&cli);
}
