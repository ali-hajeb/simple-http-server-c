#include "../include/socket.h"
#include "../include/request.h"
#include "../include/router.h"
#include "../include/file_manager.h"
#include "../include/utils.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

void* get_server_ip(struct sockaddr* server_addr) {
    if (server_addr->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) server_addr)->sin_addr);
    } else {
        return &(((struct sockaddr_in6*) server_addr)->sin6_addr);
    }
}

void* get_server_port(struct sockaddr* server_addr) {
    if (server_addr->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) server_addr)->sin_port);
    } else {
        return &(((struct sockaddr_in6*) server_addr)->sin6_port);
    }
}

void get_server_address(struct addrinfo* server, char* host, char* port) {
    inet_ntop(server->ai_family, get_server_ip(server->ai_addr), host, INET6_ADDRSTRLEN);
    if (port != NULL) inet_ntop(server->ai_family, get_server_port(server->ai_addr), port, 6);
}

int init_tcp_server_address(struct addrinfo* hints, struct addrinfo** res,
                            const char* service, int address_family) {
    if (service == NULL) {
        err("init_tcp_server_address", "Port is not defined!");
        return 1;
    }
    memset(hints, 0, sizeof(struct addrinfo));
    hints->ai_family = address_family;
    hints->ai_flags = AI_PASSIVE;
    hints->ai_socktype = SOCK_STREAM;

    int result = getaddrinfo(NULL, service, hints, res);
    if (result != 0) {
        err("init_tcp_server", gai_strerror(result));
    }
    return result;
}

int init_socket(struct addrinfo* res, char* host) {
    int socket_fd = -1;
    int yes = 1;
    struct addrinfo* ptr;
    for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {
        if ((socket_fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == -1) {
            err("init_socket", "Unable to create socket!");
            continue;
        }

        if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        int status = -1;
        if ((status = bind(socket_fd, ptr->ai_addr, ptr->ai_addrlen) == -1)) {
            err("init_socket", "Unable to bind socket!");
            continue;
        }

        get_server_address(ptr, host, NULL);

        break;
    }

    freeaddrinfo(res);

    if (ptr == NULL) {
        err("init_server", "Unable to find suitable socket!");
        return -1;
    }

    return socket_fd;
}

int start_server(int socket_fd, int queue_size, const char* host, const char* port) {
    int status = -1;
    if ((status = listen(socket_fd, queue_size))) {
        err("start_server", "Unable to bind socket!");
        return -1;
    }

    printf("LISTENING ON %s:%s...\n", host, port);

    struct sockaddr_storage client;
    socklen_t client_size = sizeof(client);
    char* request_buffer = malloc(MAX_REQ_BUFFER_SIZE);


    List routes = {0, NULL};
    setup_routes(&routes);

    HashTable file_table;
    init_hash_table(&file_table, FILE_TABLE_SIZE);
    load_files(DEFAULT_SERVER_PATH, &file_table);

    // int i = 0;
    // while (i < 2) {
    while (1) { 
        int client_fd;
        if ((client_fd = accept(socket_fd, (struct sockaddr*) &client, &client_size)) == -1) {
            err("start_server", "Unable to establish a connection with the client!");
            continue;
        }

        ssize_t recieved_bytes = recv(client_fd, request_buffer, MAX_REQ_BUFFER_SIZE - 1, 0);
        if (recieved_bytes == -1) {
            // err("start_server", "Unable to recieve request data from client!");
            printf("\t[CLIENT#%d] Unable to recieve request data from client!\n", client_fd);
            continue;
        } else if (recieved_bytes == 0) {
            printf("\t[CLIENT#%d] DISCONNECTED!\n", client_fd);
            continue;
        }
        request_buffer[recieved_bytes] = '\0';

        printf("----------------\n");
        HTTPRequest req;
        req.body = NULL;

        parse_header(&req, request_buffer);
        print_http_req(&req);

        router(&routes, &req, &client_fd, &file_table);

        free_http_req(&req);
        close(client_fd);
        // i++;
    }

    free_file_table(&file_table, FILE_TABLE_SIZE);
    free_list(&routes);
    free(request_buffer);
    return 1;
}
