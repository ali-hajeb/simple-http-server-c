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

ssize_t load_page(unsigned char** body, const char* page_path);
char* get_content_type(const char* extension);
int send_response(int* client_fd, HTTPResponseHeader* res_header, unsigned char* body, 
                  size_t body_size, const char* content_type);
int setup_routes(List* route_list, Route routes[], size_t route_count);
int router(List* route_list, HTTPRequest* req, int* client_fd, HashTable* file_table);
void home_route_handler(int* client_fd, HTTPRequest* req);
void posts_route_handler(int* client_fd, HTTPRequest* req);
void not_found_route_handler(int* client_fd, HTTPRequest* req);
int undefined_route_handler(int* client_fd, HTTPRequest* req, HashTable* file_table);
char* generate_route_key(const char* method, const char* path);
#endif
