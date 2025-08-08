#include "include/socket.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("USAGE: %s [port]\n", argv[0]);
        return 1;
    }

    const char* port = argv[1];
    struct addrinfo hints;
    struct addrinfo* res;
    int result = -1;
    if ((result = init_tcp_server_address(&hints, &res, port, AF_INET)) != 0) {
        exit(1);
    }

    char host[INET6_ADDRSTRLEN];
    int socket_fd = -1;
    if ((socket_fd = init_socket(res, host)) == -1) {
        exit(1);
    }

    if ((result = start_server(socket_fd, 10, host, port)) == -1) {
        close(socket_fd);
        exit(1);
    }

    close(socket_fd);
    return 0;
}
