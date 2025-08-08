#ifndef ROUTER_H
#define ROUTER_H
#include "request.h"
#include "linked_list.h"
#include "hash.h"

typedef struct route {
    char* path;
    char method[8];
    void (*handler)(int* client_fd, HTTPRequest* req);
} Route;

int setup_routes(List* routes);
int router(List* route_list, HTTPRequest* req, int* client_fd, HashTable* file_table);
void home_route_handler(int* client_fd, HTTPRequest* req);
void not_found_route_handler(int* client_fd, HTTPRequest* req);
int undefined_route_handler(int* client_fd, HTTPRequest* req, HashTable* file_table);
char* generate_route_key(const char* method, const char* path);
#endif
