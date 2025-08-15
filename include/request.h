#ifndef REQUEST_H
#define REQUEST_H
#include "buffer.h"
#include "linked_list.h"

#include <stdio.h>
#include <time.h>

#define DATE_BUFFER_SIZE 64

typedef struct {
    size_t size;
    size_t max_size;
    char** lines;
} LineArray;

typedef struct {
    char* path;
    List* header_fields;
    char http_version[11]; // HTTP/XX.XX
    char method[8]; // GET, HEAD, PUT, POST, PATCH, DELETE, CONNECT, OPTIONS, TRACE
} HTTPRequestHeader;

typedef struct {
    HTTPRequestHeader http_header;
    unsigned char* body;
} HTTPRequest;

typedef struct {
    List* header_fields;
    short code;
    char http_version[11];
    char desc[27]; // OK, NOT FOUND, ETC.
    char date[DATE_BUFFER_SIZE];
} HTTPResponseHeader;

typedef struct {
    HTTPResponseHeader http_header;
    unsigned char* body;
} HTTPResponse;

int init_lines(LineArray* lines, size_t initial_size);
size_t insert_line(LineArray* lines, const char* data, size_t size);
void free_lines(LineArray *line_arr);
void print_http_req(HTTPRequest* req);
void free_http_req(HTTPRequest* req);
int parse_header(HTTPRequest* req, const char* req_data);
int generate_http_date(const time_t* time, char* date_string);
// int http_response_to_string(HTTPResponse* res, char** res_string);
ssize_t http_response_to_string(HTTPResponse* res, StringBuffer* res_string);
int create_response();

#endif
