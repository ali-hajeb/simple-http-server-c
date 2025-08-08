#ifndef SOCKET_H
#define SOCKET_H

#include <netdb.h>

#define KB (1 << 10) //1024
#define MAX_REQ_BUFFER_SIZE (sizeof(char) * 4 * KB)

void* get_server_ip(struct sockaddr* server_addr);
void* get_server_port(struct sockaddr* server_addr);
void get_server_address(struct addrinfo* server, char* host, char* port);
int init_tcp_server_address(struct addrinfo* hints, struct addrinfo** res, const char* service, int address_family);
int init_socket(struct addrinfo* res, char* host);
int start_server(int socket_fd, int queue_size, const char* host, const char* port);

#endif
