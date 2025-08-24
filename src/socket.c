#include "../include/socket.h"
#include "../include/request.h"
#include "../include/router.h"
#include "../include/polls.h"
#include "../include/utils.h"

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

                int client_fd = pfds->items[i].fd;
                HTTPRequest req = {{0}, NULL};
                ssize_t received_bytes = handle_client_data(client_fd, &req);
                if (received_bytes <= 0) {
                    pfds_del(pfds, i);
                    i--;
                    printf("Client fd %d: read failed or closed (bytes=%zd)\n", client_fd, received_bytes);
                    free_http_req(&req);
                    continue;
                }

                printf("Client fd %d: Received %zd bytes\n", client_fd, received_bytes);

                router(server->routes, &req, &client_fd, server->file_table);

                pfds_del(pfds, i);
                i--;
                free_http_req(&req);
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

/*
 * Function: handle_new_connection
 *
 * -------------------------------
 *
 *  Handles new coming connection and adds it to the list.
 *
 *  pfds: pointer to the poll list.
 *  listener_fd: listener socket file descriptor.
 *
 *  returns: if failed (-1), on success (1).
 */
int handle_new_connection(PollFd* pfds, int listener_fd) {
    if (pfds == NULL) {
        return -1;
    }

    struct sockaddr_storage client_addr;
    socklen_t client_size = sizeof(client_addr);

    int client_fd = accept(listener_fd, (struct sockaddr*) &client_addr, &client_size);
    if (client_fd == -1) {
        err("handle_new_connection", "Unable to establish a connection with the client!");
        return -1;
    }

    pfds_add(pfds, client_fd);
    return client_fd;
}

/*
 * Function: handle_client_data
 *
 * ----------------------------
 *
 *  receives client's request data.
 *
 *  client_fd: client's file descriptor.
 *  req: pointer to a empty initiated http request.
 *
 *  returns: received bytes. if failed (-1).
 */
ssize_t handle_client_data(int client_fd, HTTPRequest* req) {
    if (client_fd < 0 || req == NULL) {
        return -1;
    }

    StringBuffer request_buffer;
    if (init_string_buffer(&request_buffer, 4096) == 0) {
        err("handle_client_data", "Unable to initialize request_buffer!");
        return -1;
    }

    // Temperory receive buffer
    size_t receive_buffer_size = 1024 * sizeof(char);
    char* receive_buffer = malloc(receive_buffer_size);
    if (receive_buffer == NULL) {
        err("handle_client_data", "Unable to allocate memory for buffer!");
        free_string_buffer(&request_buffer);
        return -1;
    }

    struct pollfd pfd = {client_fd, POLLIN, 0};
    int header_end_found = 0;
    size_t content_length = 0;
    size_t body_received = 0;
    size_t total_received_bytes = 0;
    while (1) {
        ssize_t received_bytes = recv(client_fd, receive_buffer, receive_buffer_size - 1, 0);
        if (received_bytes <= 0) {
            if (received_bytes == -1) {
                printf("\t[CLIENT#%d] Unable to receive request data from client!\n", client_fd);
            } else if (received_bytes == 0) {
                printf("\t[CLIENT#%d] Disconnected!\n", client_fd);
            }
            free(receive_buffer);
            free_string_buffer(&request_buffer);
            return received_bytes;
        }

        if (write_to_string_buffer(&request_buffer, receive_buffer, received_bytes) == -1) {
            free(receive_buffer);
            free_string_buffer(&request_buffer);
            return -1;
        }

        total_received_bytes += received_bytes;

        // Check for header end (\r\n\r\n)
        if (!header_end_found) {
            char* header_end = strstr(request_buffer.data, "\r\n\r\n");
            if (header_end) {
                header_end_found = 1;
                if (parse_header(req, request_buffer.data) == -1) {
                    err("handle_client_data", "Failed to parse headers for client");
                    free_string_buffer(&request_buffer);
                    return -1;
                }
                ListItem* content_length_field = list_get_item(req->http_header.header_fields, "Content-Length");
                if (content_length_field) {
                    content_length = strtoull((char*) content_length_field->value, NULL, 10);
                }
                body_received = request_buffer.size - (header_end - request_buffer.data + 4);
            }
        }

        // Check if full request is received
        if (header_end_found && body_received == content_length) {
            break;
        }

        int poll_result = poll(&pfd, 1, 5000);
        if (poll_result <= 0) {
            if (poll_result == -1) {
                err("handle_cliend_data", "Unable to wait for more data!");
                break;
            } else if (poll_result == 0) {
                break;
            }
        }
    }

    free(receive_buffer);
    free_string_buffer(&request_buffer);
    return total_received_bytes;
}
