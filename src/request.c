#include "../include/request.h"
#include "../include/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int init_lines(LineArray* line_arr, size_t initial_size) {
    if (line_arr == NULL) {
        return -1;
    }

    line_arr->lines = malloc(sizeof(char*) * initial_size);
    if (line_arr->lines == NULL) {
        err("init_lines", "Unable to allocate memory for lines!");
        return -1;
    }
    // Initialize pointers to NULL
    for (size_t i = 0; i < initial_size; i++) {
        line_arr->lines[i] = NULL;
    }

    line_arr->max_size = initial_size;
    line_arr->size = 0;
    return 0;
}

size_t insert_line(LineArray* line_arr, const char* data, size_t size) {
    if (line_arr == NULL || line_arr->lines == NULL) {
        return -1;
    }

    // check overflow
    if (line_arr->size >= line_arr->max_size) {
        // increase line_arr size
        size_t new_size =  2 * line_arr->max_size;
        // printf("new_size = %zu; 2 * %zu -> %zu : %p->%p\n", new_size, line_arr->max_size, new_size * sizeof(char*), line_arr->lines, *(line_arr->lines));
        line_arr->lines = realloc(line_arr->lines, new_size * sizeof(char*));
        if (line_arr->lines == NULL) {
            err("init_lines", "Unable to allocate memory for lines!");
            return -1;
        }
        line_arr->max_size = new_size;

        // Initialize new pointers to NULL
        for (size_t i = line_arr->size; i < line_arr->max_size; i++) {
            line_arr->lines[i] = NULL;
        }
    }

    // insert new line
    line_arr->lines[line_arr->size] = malloc(size + 1);
    if (line_arr->lines[line_arr->size] == NULL) {
        err("init_lines", "Unable to allocate memory for the new line!");
        return -1;
    }
    strncpy(line_arr->lines[line_arr->size], data, size);
    line_arr->lines[line_arr->size++][size] = '\0';
    return strlen(line_arr->lines[line_arr->size - 1]);
}

void free_lines(LineArray *line_arr) {
    if (line_arr == NULL || line_arr->lines == NULL) {
        return;
    }

    // Free each string
    for (size_t i = 0; i < line_arr->size; i++) {
        free(line_arr->lines[i]);
    }

    // Free the array of pointers
    free(line_arr->lines);
    line_arr->lines = NULL;
    line_arr->size = 0;
    line_arr->max_size = 0;
}

void print_http_req(HTTPRequest* req) {
    HTTPRequestHeader* req_header = &req->http_header;
    printf("Method: [%s]\n", req_header->method);
    printf("URI: [%s]\n", req_header->path);
    printf("HTTP Version: [%s]\n", req_header->http_version);

    for (ListItem* i = req_header->header_fields->items; i != NULL; i = i->next) {
        printf("[%s]:[%s]\n", i->key, (char*) i->value);
    }
}

size_t http_req_to_LineArray(const char* req_raw, LineArray* line_arr) {
    short line_condition = 0; // if = 2, line has ended with '\r\n'
    size_t line_size = 0;
    const char* line_start = req_raw;

    for (const char* i_ptr = req_raw; i_ptr != NULL && *i_ptr != '\0'; i_ptr++) {
        if (*i_ptr == '\r') {
            line_condition = 1;
            continue;
        } else if (*i_ptr == '\n' && line_condition == 1) {
            line_condition = 2;
        } else {
            line_condition = 0;
        }

        if (line_condition == 2 || *(i_ptr + 1) == '\0') {
            if (line_size > 0) {
                if (insert_line(line_arr, line_start, line_size) != line_size) {
                    err("http_req_to_LineArray", "Unable to insert new line!");
                    printf("\tline: %zu\n", line_arr->size + 1);
                    continue;
                }
                line_condition = 0;
                line_size = 0;
                line_start = i_ptr + 1;
                continue;
            }
            // const char* line_beg = i_ptr - line_size - 1;
            // printf("line size: %zu\n", line_size);
            // int result = insert_line(line_arr, line_beg, line_size);
            // if (result == -1) {
            //     err("http_req_to_LineArray", "Unable to insert the new line!");
            //     printf("\tline: %zu\n", line_arr->size + 1);
            //     continue;
            // }
            // line_condition = 0;
            // line_size = 0;
            // continue;
        }
        line_size++;
    }

    return line_arr->size;
}

int parse_request_line(const char* req_line, HTTPRequestHeader* req_header) {
    // printf("line: %s\n", req_line);
    size_t keyword_size = 0;
    int keyword_counter = 0;
    for (const char* i = req_line; i != NULL && *i != '\0'; i++) {
        if (*i == ' ' || *(i + 1) == '\0') {
            // ignore trailing whitespaces
            if (keyword_size == 0) continue;

            keyword_counter++;

            char* dest;
            size_t dest_size;
            if (keyword_counter == 1) {
                dest = req_header->method;
                dest_size = sizeof(req_header->method);
            } else if (keyword_counter == 2) {
                req_header->path = malloc(sizeof(char) * keyword_size + 1);
                if (req_header->path  == NULL) {
                    err("parse_header", "Unable to allocate memory for path!");
                    return -1;
                }
                dest = req_header->path;
                dest_size = sizeof(char) * keyword_size + 1;
            } else if (keyword_counter == 3) {
                dest = req_header->http_version;
                dest_size = sizeof(req_header->http_version);
            } else {
                break;
            }

            if (keyword_size > dest_size) {
                if (keyword_counter == 2) {
                    free(dest);
                    err("parse_request_line", "Keyword is larger than destination size!");
                    return -1;
                }
            }

            const char* keyword_beg = i - keyword_size;
            strncpy(dest, keyword_beg, keyword_size + 1);

            // if i is near the end of cstring, include i
            if (*(i + 1) == '\0') keyword_size++; 
            dest[keyword_size] = '\0';
            keyword_size = 0;
            continue;
        }
        keyword_size++;
    }

    if (keyword_counter != 3) {
        err("parse_request_line", "Invalid request line!");
        return -1;
    }
    return 1;
}

int parse_header_fields(const LineArray* line_arr, List* header_fields) {
    for (size_t i = 1; i < line_arr->size; i++) {
        // printf("[%s]\n", line_arr->lines[i]);

        // Check for empty line
        if (strlen(line_arr->lines[i]) == 0) {
            break;
        }

        size_t value_size = 0;
        char* key = NULL;
        char* value = NULL;
        char* j_ptr;

        // get header key
        for (j_ptr = line_arr->lines[i]; j_ptr != NULL && *j_ptr != '\0'; j_ptr++) {
            if (*j_ptr == ':') {
                char* key_beg = j_ptr - value_size;

                key = malloc(sizeof(char) * value_size + 1);
                if (key == NULL) {
                    err("parse_header", "Unable to allocate memory for header key!");
                    return -1;
                }

                // store field-name
                strncpy(key, key_beg, value_size);
                key[value_size] = '\0';

                // prepare for recording value of the field
                value_size = 0;
                j_ptr++;
                break;
            }
            value_size++;
        }

        // check if field-name is stored (i.e incomplete requests)
        if (key == NULL) {
            err("parse_header_fields", "Invalid Header!");
            printf("\tline[%zu]: %s\n", i, line_arr->lines[i]);
            continue;
        }

        // get field value
        int beg_ows_counter = 0; // number of optional whitespaces in the begining
        int end_ows_counter = 0; // number of optional whitespaces at the end
        int recording_beg_ows = 1; // flag for counting whitespaces in the begining
        for (; j_ptr != NULL && *j_ptr != '\0'; j_ptr++) {
            // trim begining and ending whitespaces
            if (*j_ptr == ' ') {
                // if ows is in the begining
                if (recording_beg_ows == 1) {
                    beg_ows_counter++;
                } else {
                    end_ows_counter++;
                }
            } else {
                // reset counter on non whitespace character
                recording_beg_ows = 0;
                end_ows_counter = 0;
            }
            value_size++;
        }

        if (value_size > 0) {
            char* value_beg = j_ptr + beg_ows_counter - value_size;
            size_t trimed_value_size = value_size - beg_ows_counter - end_ows_counter;

            value = malloc(sizeof(char) * trimed_value_size + 1);
            if (value == NULL) {
                err("parse_header", "Unable to allocate memory for header key!");
                free(key);
                return -1;
            }

            // store field-value
            strncpy(value, value_beg, trimed_value_size);
            value[trimed_value_size] = '\0';
        }

        // Add field to the list
        if (list_set_item(header_fields, key, value, value_size) == -1) {
            err("parse_header", "Unable to insert the item!");
            printf("\tline[%zu]: %s\n", i, line_arr->lines[i]);
            free(key);
            free(value);
            continue;
        }
        free(key);
        free(value);
    }

    return header_fields->size;
}

int parse_header(HTTPRequest* req, const char* req_data) {
    LineArray line_arr;
    int result = init_lines(&line_arr, 4);
    if (result == -1) {
        return -1;
    }

    http_req_to_LineArray(req_data, &line_arr);

    if (line_arr.size == 0) {
        err("parse_header", "No lines parsed!");
        free_lines(&line_arr);
        return -1;
    }

    // Parse first line
    result = parse_request_line(line_arr.lines[0], &req->http_header);
    if (result == -1) {
        free_lines(&line_arr);
        return -1;
    }

    List* header_fields = malloc(sizeof(List));
    if (header_fields == NULL) {
        err("parse_header", "Unable to allocate memory for header fields!");
        free_lines(&line_arr);
        return -1;
    }
    header_fields->items = NULL;
    header_fields->size = 0;

    // Parse the rest of the fields
    result = parse_header_fields(&line_arr, header_fields);
    if (result == -1) {
        free_lines(&line_arr);
        free(header_fields);
        return -1;
    }

    req->http_header.header_fields = header_fields;

    free_lines(&line_arr);
    return 1;
}

void free_http_req(HTTPRequest* req) {
    if (req->body != NULL) {
        free(req->body);
    }

    HTTPRequestHeader* req_header = &req->http_header;
    if (req_header->path != NULL) {
        free(req_header->path);
    }
    if (req_header->header_fields != NULL) {
        free_list(req_header->header_fields);
        free(req_header->header_fields);
    }
}

ssize_t http_response_to_string(HTTPResponse* res, StringBuffer* res_string) {
    if (res == NULL) {
        return -1;
    }

    HTTPResponseHeader* res_header = &res->http_header;

    char* line = malloc(1024 * sizeof(char));
    if (line == NULL) {
        err("http_response_to_string", "Unable to allocate memory for line buffer!");
        return -1;
    }


    int written_bytes = sprintf(line, "%s %d %s\r\n", res_header->http_version, res_header->code, res_header->desc);
    // printf("1. [%s]\n", line);
    write_to_string_buffer(res_string, line, strlen(line));
    // printf("1-1. [%s]\n", res_string->data);

    written_bytes += sprintf(line, "%s\r\n", res_header->date);
    // printf("2. [%s]\n", line);
    write_to_string_buffer(res_string, line, strlen(line));
    // printf("2-1. [%s]\n", res_string->data);

    for (ListItem* field = res_header->header_fields->items; field != NULL; field = field->next) {
        written_bytes += snprintf(line, 1024, "%s: %s\r\n", field->key, (char*) field->value);
        // printf("3. [%s]\n", line);
        write_to_string_buffer(res_string, line, strlen(line));
        // printf("3-1. [%s]\n", res_string->data);
    }

    written_bytes += sprintf(line, "\r\n");
    // printf("4. [%s]\n", line);
    write_to_string_buffer(res_string, line, strlen(line));
    // printf("4-1. [%s]\n", res_string->data);
    // print_buffer(res_string->data, res_string->max_size, 8);

    ListItem* content_length = list_get_item(res_header->header_fields, "Content-Length");
    if (content_length == NULL) {
        err("http_response_to_string", "No content-length!");
        free(line);
        return -1;
    }

    size_t body_size = strtoull((char*) content_length->value, NULL, 10);
    // printf("--- %zu\n", body_size);

    write_to_string_buffer(res_string, res->body, body_size);
    // printf("\n\r\n4.[%s]\n", res_string->data);

    free(line);
    return res_string->size;
}

// int prepare_http_response(int client_fd, HTTPResponse* res) {
//     StringBuffer response_string;
//     init_string_buffer(&response_string, 256);
//
//     if (http_response_to_string(res, &response_string) == -1) {
//         err("home_route_handler", "Unable to convert response to string!");
//         return -1;
//     }
//
//     int status = send(*client_fd, response_string.data, response_string.size, 0);
//     if (status == -1) {
//         err("home_route_handler", "Unable to respond to request!");
//         printf("\t%d,\n%s\n", status, response_string.data);
//     }
//
//     free_string_buffer(&response_string);
// }

int generate_http_date(const time_t* timer, char* date_string) {
    struct tm* gmt = gmtime(timer);

    // char* date_string = malloc(DATE_BUFFER_SIZE * sizeof(char));
    // if (date_string == NULL) {
    //     err("generate_http_date", "Unable to allocate memory for date string!");
    //     return NULL;
    // }

    size_t result = strftime(date_string, DATE_BUFFER_SIZE * sizeof(char), 
                          "Date: %a, %d %b %Y %H:%M:%S GMT", gmt);
    if (result == 0) {
        err("generate_http_date", "Unable to generate date string (overflow)!");
    }

    return result;
}
