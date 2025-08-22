#include "include/socket.h"
#include "include/linked_list.h"
#include "include/hash.h"
#include "include/router.h"
#include "include/file_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("USAGE: %s [port]\n", argv[0]);
        return 1;
    }

    Server server = {{0}, {0}, -1, NULL, NULL};
    strncpy(server.port, argv[1], 6);
    struct addrinfo hints;
    struct addrinfo* res;
    int result = -1;
    if ((result = init_tcp_server_address(&hints, &res, server.port, AF_INET)) != 0) {
        exit(1);
    }

    if ((server.socket_fd = init_socket(res, server.host)) == -1) {
        exit(1);
    }


    List routes = {0, NULL};
    Route route_arr[] = {
        {"/", "GET", home_route_handler},
        {"/posts", "GET", posts_route_handler},
    };
    size_t route_count = sizeof(route_arr) / sizeof(route_arr[0]);
    result = setup_routes(&routes, route_arr, route_count);
    if (result < (int) route_count) {
        close(server.socket_fd);
        exit(1);
    }
    server.routes = &routes;

    FileTable file_table;
    result = init_hash_table(&file_table, FILE_TABLE_SIZE);
    if (result == -1) {
        close(server.socket_fd);
        free_list(&routes);
        exit(1);
    }

    result = load_files(DEFAULT_SERVER_PATH, &file_table);
    if (result == -1) {
        close(server.socket_fd);
        free_file_table(&file_table);
        free_list(&routes);
        exit(1);
    }
    server.file_table = &file_table;

    if ((result = start_server(&server, 128)) == -1) {
        close(server.socket_fd);
        free_file_table(&file_table);
        free_list(&routes);
        exit(1);
    }

    close(server.socket_fd);
    free_file_table(&file_table);
    free_list(&routes);
    return 0;
}
