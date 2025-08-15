#include "../include/socket.h"
#include "../include/request.h"
#include "../include/router.h"
#include "../include/polls.h"
#include "../include/utils.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/poll.h>
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

int process_connections(PollFd* pfds, Server* server) {
    if (pfds == NULL || server == NULL) {
        return -1;
    }

    for (size_t i = 0; i < pfds->size; i++) {
        if (pfds->items[i].revents & (POLLIN | POLLHUP)) {
            printf("Event on fd %d: revents=%d\n", pfds->items[i].fd, pfds->items[i].revents);
            if (pfds->items[i].fd == server->socket_fd) {
                handle_new_connection(pfds, server->socket_fd);
                printf("New connection on server socket\n");
            } else {
                printf("Client data on fd %d\n", pfds->items[i].fd);
                char* request_buffer = malloc(MAX_REQ_BUFFER_SIZE);
                if (request_buffer == NULL) {
                    err("process_connections", "Unable to allocate memory for buffer!");
                    // return -1;
                    continue;
                }

                int client_fd = pfds->items[i].fd;
                ssize_t received_bytes = handle_client_data(client_fd, request_buffer, MAX_REQ_BUFFER_SIZE);
                if (received_bytes <= 0) {
                    pfds_del(pfds, i);
                    i--;
                    printf("Client fd %d: read failed or closed (bytes=%zd)\n", client_fd, received_bytes);
                    free(request_buffer);
                    continue;
                }

                printf("Client fd %d: Received %zd bytes\n", client_fd, received_bytes);
                HTTPRequest req;
                req.body = NULL;

                parse_header(&req, request_buffer);
                // print_http_req(&req);

                router(server->routes, &req, &client_fd, server->file_table);

                pfds_del(pfds, i);
                i--;
                free_http_req(&req);
                free(request_buffer);
                printf("----------------\n");
            }
        }
    }
    return 1;
}

int start_server(Server* server, int queue_size) {
    int status = -1;
    if ((status = listen(server->socket_fd, queue_size))) {
        err("start_server", "Unable to bind socket!");
        return -1;
    }

    printf("LISTENING ON %s:%s...\n", server->host, server->port);

    PollFd pfds;
    if (init_pfds(&pfds, 10) == -1) {
        return -1;
    }

    pfds_add(&pfds, server->socket_fd);

    // int i = 0;
    while (1) {
        int poll_count = poll(pfds.items, pfds.size, -1);

        if (poll_count == -1) {
            err("start_server", "Poll Error!");
            break;
        }

        process_connections(&pfds, server);
        // if (i++ == 4) break;
    }
    free_pfds(&pfds);
    return 1;
}
