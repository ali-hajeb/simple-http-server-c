#ifndef SOCKET_H
#define SOCKET_H
#include "linked_list.h"
#include "polls.h"
#include "hash.h"
#include "request.h"

#include <netdb.h>
#include <netinet/in.h>

#define KB (1 << 10) //1024
#define MAX_REQ_BUFFER_SIZE (sizeof(char) * 4 * KB)

typedef struct {
    char host[INET6_ADDRSTRLEN];
    char port[6];
    int socket_fd;
    List* routes;
    HashTable* file_table;
} Server;

int free_server(Server* server);
void* get_server_ip(struct sockaddr* server_addr);
void* get_server_port(struct sockaddr* server_addr);
void get_server_address(struct addrinfo* server, char* host, char* port);
int init_tcp_server_address(struct addrinfo* hints, struct addrinfo** res, const char* service, int address_family);
int init_socket(struct addrinfo* res, char* host);
int start_server(Server* server, int queue_size);
int process_connections(PollFd* pfds, Server* server);

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
int handle_new_connection(PollFd* pfds, int listener_fd);

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
ssize_t handle_client_data(int client_fd, HTTPRequest* req);
#endif
