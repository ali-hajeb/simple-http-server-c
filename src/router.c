#include "../include/router.h"
#include "../include/buffer.h"
#include "../include/file_manager.h"
#include "../include/utils.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

char* generate_route_key(const char* method, const char* path) {
    size_t route_key_size = strlen(path) + strlen(method) + 2;
    char* route_key = malloc(route_key_size * sizeof(char));
    if (route_key == NULL) {
        err("setup_routes", "Unable to allocate memory for route key!");
        printf("\t%s %s\n", method, path);
        return NULL;
    }

    int written_bytes = sprintf(route_key, "%s:%s", method, path);
    // printf("[][] %d vs %zu\n", written_bytes, route_key_size);
    return route_key;
}

int setup_routes(List* route_list) {
    Route routes[] = {
        {"/", "GET", home_route_handler},
    };

    size_t route_count = sizeof(routes) / sizeof(routes[0]);
    size_t failed_routes = 0;

    for (size_t i = 0; i < route_count; i++) {
        char* route_key = generate_route_key(routes[i].method, routes[i].path);
        if (route_key == NULL) {
            failed_routes++;
            continue;
        }

        if (list_set_item(route_list, route_key, &routes[i], sizeof(Route)) == -1) {
            err("setup_routes", "Unable to add the route:");
            printf("\tpath: %s\n\tmethod: %s\n", routes[i].path, routes[i].method);
            free(route_key);
            return -1;
        }
        free(route_key);
    }

    return route_count - failed_routes;
}

int router(List* route_list, HTTPRequest* req, int* client_fd, HashTable* file_table) {
    char* route_key = generate_route_key(req->http_header.method, req->http_header.path);
    if (route_key == NULL) {
        return -1;
    }

    ListItem* route = list_get_item(route_list, route_key);
    if (route != NULL) {
        Route* selected_route = (Route*) route->value;
        selected_route->handler(client_fd, req);
    } else {
        undefined_route_handler(client_fd, req, file_table);
    }
    free(route_key);
    return 1;
}

int undefined_route_handler(int* client_fd, HTTPRequest* req, HashTable* file_table) {
    char* requested_path = NULL;
    int requested_path_size = req_path_to_local(req->http_header.path, strlen(req->http_header.path), &requested_path);

    if (requested_path_size < 1) {
        return -1;
    }

    int hash_value = hash(requested_path, requested_path_size, FILE_TABLE_SIZE);
    HashEntry* file_entry = file_table->entry[hash_value];
    if (file_entry) {
        File* file = (File*) file_entry->data;
        List header_fields = {0, NULL};
        HTTPResponseHeader res_header = {
            &header_fields, 
            200, 
            "HTTP/1.1", 
            "OK", 
            ""
        };

        time_t raw_time;
        time(&raw_time);
        generate_http_date(&raw_time, res_header.date);

        unsigned char* body = NULL;
        size_t read_bytes = read_file_content(file->path, &body);
        char content_length[21] = {'\0'};
        sprintf(content_length, "%zu", read_bytes);
        list_set_item(&header_fields, "Content-Length", &content_length, strlen(content_length) + 1);

        char* content_type;
        if (strcmp(file->extension, "html") == 0) {
            content_type = "text/html; charset=UTF-8"; 
        } else if (strcmp(file->extension, "css") == 0) {
            content_type = "text/css";
        } else {
            content_type = "application/octet-stream";
        }
        list_set_item(&header_fields, "Content-Type", content_type, strlen(content_type) + 1);

        // char* response_string = NULL;
        HTTPResponse res = {res_header, body};

        StringBuffer response_string;
        init_string_buffer(&response_string, 256);

        if (http_response_to_string(&res, &response_string) == -1) {
            err("undefined_route_handler", "Unable to convert response to string!");
            free_string_buffer(&response_string);
            free_list(&header_fields);
            free(body);
            return -1;
        }

        int status = send(*client_fd, response_string.data, response_string.size, 0);
        if (status == -1) {
            err("undefined_route_handler", "Unable to respond to request!");
            printf("\t%d,\n%s\n", status, response_string.data);
        }

        free_string_buffer(&response_string);
        free_list(&header_fields);
        free(body);
    } else {
        not_found_route_handler(client_fd, req);
    }
    free(requested_path);
    return 1;
}

void home_route_handler(int* client_fd, HTTPRequest* req) {
    List header_fields = {0, NULL};
    HTTPResponseHeader res_header = {
        &header_fields, 
        200, 
        "HTTP/1.1", 
        "OK", 
        ""
    };

    time_t raw_time;
    time(&raw_time);
    generate_http_date(&raw_time, res_header.date);

    unsigned char* body = NULL;
    size_t file_path_size = strlen(DEFAULT_SERVER_PATH) + strlen("/index.html") + 1;
    char* file_path = malloc(file_path_size * sizeof(char));
    if (file_path == NULL) {
        err("home_route_handler", "Unable to allocate memory for file path!");
        return;
    }

    snprintf(file_path, file_path_size, "%s/index.html", DEFAULT_SERVER_PATH);
    size_t read_bytes = read_file_content(file_path, &body);

    char content_length[21] = {'\0'};
    sprintf(content_length, "%zu", read_bytes);
    list_set_item(&header_fields, "Content-Length", &content_length, strlen(content_length) + 1);

    char* content_type =  "text/html; charset=UTF-8";
    list_set_item(&header_fields, "Content-Type", content_type, strlen(content_type) + 1);

    // char* response_string = NULL;
    HTTPResponse res = {res_header, body};

    StringBuffer response_string;
    init_string_buffer(&response_string, 256);

    if (http_response_to_string(&res, &response_string) == -1) {
        err("home_route_handler", "Unable to convert response to string!");
        return;
    }

    int status = send(*client_fd, response_string.data, response_string.size, 0);
    if (status == -1) {
        err("home_route_handler", "Unable to respond to request!");
        printf("\t%d,\n%s\n", status, response_string.data);
    }

    free_string_buffer(&response_string);
    free_list(&header_fields);
    return;
}

void not_found_route_handler(int* client_fd, HTTPRequest* req) {
    List header_fields = {0, NULL};
    HTTPResponseHeader res_header = {
        &header_fields, 
        404, 
        "HTTP/1.1", 
        "Not Found", 
        ""
    };

    time_t raw_time;
    time(&raw_time);
    generate_http_date(&raw_time, res_header.date);

    char* body = "<html>"
        "<head><title>404 - Not Found</title></head>"
        "<body><h1>Oopse! You're Lost!</h1><h2>Page Not Found</h2><hr /></body>"
        "</html>";
    // size_t content_length = strlen(body);
    char content_length[21] = {0};
    sprintf(content_length, "%zu", strlen(body));
    printf("%s %zu --------\n", content_length, strlen(body));
    list_set_item(&header_fields, "Content-Length", &content_length, sizeof(content_length));

    char* content_type =  "text/html; charset=UTF-8";
    list_set_item(&header_fields, "Content-Type", content_type, strlen(content_type));

    // char* response_string = NULL;
    HTTPResponse res = {res_header, body};
    StringBuffer response_string;
    init_string_buffer(&response_string, 256);

    if (http_response_to_string(&res, &response_string) == -1) {
        err("router", "Unable to convert response to string!");
        return;
    }

    int status = send(*client_fd, response_string.data, response_string.size, 0);
    if (status == -1) {
        err("router", "Unable to respond to request!");
        printf("\t%d,\n%s\n", status, response_string.data);
    }

    free_string_buffer(&response_string);
    free_list(&header_fields);
}
