#include "../include/request.h"
#include "../include/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
    return 1;
}

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
ssize_t insert_line(LineArray* line_arr, const char* data, size_t size) {
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
            err("insert_line", "Unable to allocate memory for lines!");
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
        err("insert_line", "Unable to allocate memory for the new line!");
        return -1;
    }
    strncpy(line_arr->lines[line_arr->size], data, size);
    line_arr->lines[line_arr->size++][size] = '\0';
    return strlen(line_arr->lines[line_arr->size - 1]);
}

/*
 * Function: free_lines
 *
 * --------------------
 *
 *  Frees line list.
 *
 *  line_arr: pointer to the line list.
 */
void free_lines(LineArray* line_arr) {
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

/*
 * Function: print_http_req
 *
 * ------------------------
 *
 *  Prints HTTP request data.
 *
 *  req: pointer to the http request.
 */
void print_http_req(HTTPRequest* req) {
    HTTPRequestHeader* req_header = &req->http_header;
    printf("Method: [%s]\n", req_header->method);
    printf("URI: [%s]\n", req_header->path);
    printf("HTTP Version: [%s]\n", req_header->http_version);

    for (ListItem* i = req_header->header_fields->items; i != NULL; i = i->next) {
        printf("[%s]:[%s]\n", i->key, (char*) i->value);
    }
}

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
size_t http_req_to_LineArray(const char* req_raw, LineArray* line_arr) {
    short line_condition = 0; // if = 2, line has ended with '\r\n'
    ssize_t line_size = 0;
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
        }
        line_size++;
    }

    return line_arr->size;
}

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

/*
 * Function: free_http_req
 *
 * -----------------------
 *
 *  Frees HTTP Request struct.
 *
 *  req: pointer to the http request struct.
 */
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
ssize_t http_response_to_string(HTTPResponse* res, StringBuffer* res_string) {
    if (res == NULL) {
        return -1;
    }

    HTTPResponseHeader* res_header = &res->http_header;

    // Temperory buffer for lines.
    size_t max_line_size = 1024 * sizeof(char);
    char* line = malloc(max_line_size);
    if (line == NULL) {
        err("http_response_to_string", "Unable to allocate memory for line buffer!");
        return -1;
    }

    // write response line
    int written_bytes = snprintf(line, max_line_size, "%s %d %s\r\n", 
                                 res_header->http_version, res_header->code, res_header->desc);
    // printf("1. [%s]\n", line);
    ssize_t result = write_to_string_buffer(res_string, line, strlen(line));
    if (result == -1) {
        err("http_response_to_string", "Unable to write to the string buffer!");
        printf("\tline: %s\n", line);
        free(line);
        return -1;
    }
    // printf("1-1. [%s]\n", res_string->data);

    // write response date
    written_bytes += snprintf(line, max_line_size,"%s\r\n", res_header->date);
    // printf("2. [%s]\n", line);
    result = write_to_string_buffer(res_string, line, strlen(line));
    if (result == -1) {
        err("http_response_to_string", "Unable to write to the string buffer!");
        printf("\tline: %s\n", line);
        free(line);
        return -1;
    }
    // printf("2-1. [%s]\n", res_string->data);

    // write headers
    for (ListItem* field = res_header->header_fields->items; field != NULL; field = field->next) {
        written_bytes += snprintf(line, max_line_size, "%s: %s\r\n", field->key, (char*) field->value);
        // printf("3. [%s]\n", line);
        result = write_to_string_buffer(res_string, line, strlen(line));
        if (result == -1) {
            err("http_response_to_string", "Unable to write to the string buffer!");
            printf("\tline: %s\n", line);
            free(line);
            return -1;
        }
        // printf("3-1. [%s]\n", res_string->data);
    }

    written_bytes += sprintf(line, "\r\n");
    // printf("4. [%s]\n", line);
    result = write_to_string_buffer(res_string, line, strlen(line));
    if (result == -1) {
        err("http_response_to_string", "Unable to write to the string buffer!");
        printf("\tline: %s\n", line);
        free(line);
        return -1;
    }
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

    // write body
    result = write_to_string_buffer(res_string, res->body, body_size);
    if (result == -1) {
        err("http_response_to_string", "Unable to write to the string buffer!");
        printf("\tline: %s\n", line);
        free(line);
        return -1;
    }
    // printf("\n\r\n4.[%s]\n", res_string->data);

    free(line);
    return res_string->size;
}

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
size_t generate_http_date(const time_t* timer, char* date_string) {
    struct tm* gmt = gmtime(timer);

    size_t result = strftime(date_string, DATE_BUFFER_SIZE * sizeof(char), 
                          "Date: %a, %d %b %Y %H:%M:%S GMT", gmt);
    if (result == 0) {
        err("generate_http_date", "Unable to generate date string (overflow)!");
    }

    return result;
}
