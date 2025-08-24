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
    char http_version[11]; // HTTP/XX.XX
    char method[8]; // GET, HEAD, PUT, POST, PATCH, DELETE, CONNECT, OPTIONS, TRACE
    char* path;
    List* header_fields;
} HTTPRequestHeader;

typedef struct {
    HTTPRequestHeader http_header;
    unsigned char* body;
} HTTPRequest;

typedef struct {
    char date[DATE_BUFFER_SIZE];
    char desc[32]; // OK, NOT FOUND, ETC.
    char http_version[11];
    List* header_fields;
    short code;
} HTTPResponseHeader;

typedef struct {
    HTTPResponseHeader http_header;
    unsigned char* body;
} HTTPResponse;

/*
 * Function: init_lines
 *
 * --------------------
 *
 *  Initiates line list.
 *
 *  line_arr: pointer to the line list.
 *  initial_size: initial list size.
 *
 *  returns: if failed (-1), on success (1).
 */
int init_lines(LineArray* line_arr, size_t initial_size);

/*
 * Function: insert_line
 *
 * ---------------------
 *
 *  Adds line to the list.
 *
 *  line_arr: pointer to the list.
 *  data: line data.
 *  size: line lenght.
 *
 *  returns: length of the inserted line.
 */
ssize_t insert_line(LineArray* line_arr, const char* data, size_t size);

/*
 * Function: free_lines
 *
 * --------------------
 *
 *  Frees line list.
 *
 *  line_arr: pointer to the line list.
 */
void free_lines(LineArray* line_arr);

/*
 * Function: print_http_req
 *
 * ------------------------
 *
 *  Prints HTTP request data.
 *
 *  req: pointer to the http request.
 */
void print_http_req(HTTPRequest* req);

/*
 * Function: http_req_to_LineArray
 *
 * -------------------------------
 *
 *  Converts raw http requests to lines.
 *
 *  req_raw: raw http request.
 *  line_arr: pointer to the line list.
 *
 *  returns: number of lines.
 */
size_t http_req_to_LineArray(const char* req_raw, LineArray* line_arr);

/*
 * Function: parse_request_line
 *
 * ----------------------------
 *
 *  Parses the first line of the HTTP request.
 *
 *  req_line: pointer to the string data of the first line.
 *  req_header: pointer to the HTTPRequestHeader struct.
 *
 *  returns: if failed (-1), on success (1).
 */
int parse_request_line(const char* req_line, HTTPRequestHeader* req_header);

/*
 * Function: parse_header_fields
 *
 * -----------------------------
 *
 *  Parse's HTTP Request headers to a list of key-values.
 *
 *  line_arr: pointer to the line list.
 *  header_fields: pointer to the header fields list.
 *
 *  returns: number of header fields. if failed (-1).
 */
int parse_header_fields(const LineArray* line_arr, List* header_fields);

/*
 * Function: parse_header
 *
 * ----------------------
 *
 *  Parses raw http request headers.
 *
 *  req: pointer to the HTTPRequest struct.
 *  req_data: pointer to the raw request data.
 *
 *  returns: if failed (-1), on success (1). 
 */
int parse_header(HTTPRequest* req, const char* req_data);

/*
 * Function: free_http_req
 *
 * -----------------------
 *
 *  Frees HTTP Request struct.
 *
 *  req: pointer to the http request struct.
 */
void free_http_req(HTTPRequest* req);

/*
 * Function: http_response_to_string
 *
 * ---------------------------------
 *
 *  Stringifies the HTTP Response struct.
 *
 *  res: pointer to the http response struct.
 *  res_string: pointer to the string buffer.
 *
 *  returns: size of response string. if failed (-1).
 */
ssize_t http_response_to_string(HTTPResponse* res, StringBuffer* res_string);

/*
 * Function: generate_http_date
 *
 * ----------------------------
 *
 *  Generates date string.
 *
 *  timer: pointer to the current time.
 *  date_string: pointer to the date string.
 *
 *  returns: date string length.
 */
size_t generate_http_date(const time_t* timer, char* date_string);
#endif
