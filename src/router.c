#include "../include/router.h"
#include "../include/buffer.h"
#include "../include/file_manager.h"
#include "../include/utils.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>

char* generate_route_key(const char* method, const char* path) {
    size_t route_key_size = strlen(path) + strlen(method) + 2;
    char* route_key = malloc(route_key_size * sizeof(char));
    if (route_key == NULL) {
        err("setup_routes", "Unable to allocate memory for route key!");
        printf("\t%s %s\n", method, path);
        return NULL;
    }

    snprintf(route_key, route_key_size, "%s:%s", method, path);
    // printf("[][] %d vs %zu\n", written_bytes, route_key_size);
    return route_key;
}

int setup_routes(List* route_list, Route routes[], size_t route_count) {
    size_t failed_routes = 0;
    for (size_t i = 0; i < route_count; i++) {
        char* route_key = generate_route_key(routes[i].method, routes[i].path);
        printf("route -> %s\n", route_key);
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

int send_response(int* client_fd, HTTPResponseHeader* res_header, unsigned char* body, size_t body_size, const char* content_type) {
    time_t raw_time;
    time(&raw_time);
    ssize_t result = generate_http_date(&raw_time, res_header->date);
    if (result == 0) {
        return -1;
    }

    char content_length[32] = {'\0'};
    snprintf(content_length, sizeof(content_length), "%zu", body_size);
    list_set_item(res_header->header_fields, "Content-Length", content_length, strlen(content_length) + 1);

    list_set_item(res_header->header_fields, "Content-Type", content_type, strlen(content_type) + 1);

    HTTPResponse res = {*res_header, body};

    StringBuffer response_string;
    init_string_buffer(&response_string, 256 + body_size);

    if (http_response_to_string(&res, &response_string) == -1) {
        err("send_response", "Unable to convert response to string!");
        free_string_buffer(&response_string);
        // free_list(&header_fields);
        return -1;
    }

    int send_status = send(*client_fd, response_string.data, response_string.size, 0);
    if (send_status == -1) {
        err("send_response", "Unable to respond to request!");
        printf("\t%d,\n%s\n", send_status, response_string.data);
    }

    free_string_buffer(&response_string);
    // free_list(&header_fields);
    return send_status != -1 ? 1 : -1;
}

char* get_content_type(const char* extension) {
    if (extension == NULL) return "application/octet-stream";
    if (strcmp(extension, "html") == 0) return "text/html; charset=UTF-8";
    if (strcmp(extension, "css") == 0) return "text/css";
    if (strcmp(extension, "js") == 0) return "application/javascript";
    if (strcmp(extension, "jpg") == 0 || strcmp(extension, "jpeg") == 0) return "image/jpeg";
    return "application/octet-stream";
}

int undefined_route_handler(int* client_fd, HTTPRequest* req, HashTable* file_table) {
    char* requested_path = NULL;
    int requested_path_size = req_path_to_local(req->http_header.path, strlen(req->http_header.path), &requested_path);

    if (requested_path_size < 1 || requested_path == NULL) {
        free(requested_path);
        return -1;
    }

    int hash_value = hash(requested_path, requested_path_size, FILE_TABLE_SIZE);
    HashEntry* file_entry = file_table->entry[hash_value];
    File* file = file_entry ? (File*) file_entry->data : NULL;

    if (file != NULL) {
        unsigned char* body = NULL;
        size_t read_bytes = read_file_content(file->path, &body);
        if (body == NULL) {
            err("undefined_route_handler", "Unable to read file content!");
            free(requested_path);
            free(body);
            return -1;
        }

        List header_fields = {0, NULL};
        HTTPResponseHeader res_header = {
            {0},
            "OK", 
            "HTTP/1.1", 
            &header_fields, 
            200, 
        };
        char* content_type = get_content_type(file->extension);
        int status = send_response(client_fd, &res_header, body, read_bytes, content_type);
        free(body);
        free(requested_path);
        free_list(&header_fields);
        return status;
    } else if (strcmp(req->http_header.method, "GET") == 0 && requested_path[requested_path_size - 1] == '/') {
        size_t index_path_size = (size_t)requested_path_size + strlen("index.html") + 1;
        char* index_path = malloc(index_path_size * sizeof(char));
        if (index_path == NULL) {
            err("undefined_route_handler", "Unable to allocate memory for index path!");
            free(requested_path);
            return -1;
        }
        snprintf(index_path, index_path_size, "%sindex.html", requested_path);
        int index_hash_value = hash(index_path, strlen(index_path), FILE_TABLE_SIZE);
        HashEntry* index_entry = file_table->entry[index_hash_value];
        file = index_entry ? (File*) index_entry->data : NULL;
        if (file != NULL) {
            unsigned char* body = NULL;
            size_t read_bytes = read_file_content(file->path, &body);
            if (body == NULL) {
                err("undefined_route_handler", "Unable to read index file content!");
                free(index_path);
                free(requested_path);
                free(body);
                return -1;
            }

            List header_fields = {0, NULL};
            HTTPResponseHeader res_header = {
                {0},
                "OK", 
                "HTTP/1.1", 
                &header_fields, 
                200, 
            };
            char* content_type = get_content_type(file->extension);
            int status = send_response(client_fd, &res_header, body, read_bytes, content_type);
            free(body);
            free(index_path);
            free(requested_path);
            free_list(&header_fields);
            return status;
        }
        free(index_path);
    }

    free(requested_path);
    not_found_route_handler(client_fd, req);
    return 1;
}

ssize_t load_page(unsigned char** body, const char* page_path) {
    size_t file_path_size = strlen(DEFAULT_SERVER_PATH) + strlen(page_path) + 1;
    char* file_path = malloc(file_path_size * sizeof(char));
    if (file_path == NULL) {
        err("home_route_handler", "Unable to allocate memory for file path!");
        return -1;
    }

    snprintf(file_path, file_path_size, "%s%s", DEFAULT_SERVER_PATH, page_path);
    size_t read_bytes = read_file_content(file_path, body);
    free(file_path);
    return read_bytes;
}

void generic_route_handler(int* client_fd, HTTPRequest* req, const char* page_path, int status_code, const char* status_desc) {
    unsigned char* body = NULL;
    ssize_t read_bytes = load_page(&body, page_path);
    if (body == NULL) {
        err("generic_route_handler", "Unable to read file content!");
        return;
    }
    List header_fields = {0, NULL};
    HTTPResponseHeader res_header = {
        .date = {0},
        .desc = *status_desc,
        .http_version = "HTTP/1.1",
        .header_fields = &header_fields,
        .code = status_code,
    };

    const char* content_type = "text/html; charset=UTF-8";
    send_response(client_fd, &res_header, body, read_bytes, content_type);
    free(body);
    free_list(&header_fields);
}

void home_route_handler(int* client_fd, HTTPRequest* req) {
    unsigned char* body = NULL;
    ssize_t read_bytes = load_page(&body, "/index.html");
    if (body == NULL) {
        err("home_route_handler", "Unable to read file content!");
        free(body);
        return;
    }

    List header_fields = {0, NULL};
    HTTPResponseHeader res_header = {
        {0},
        "OK", 
        "HTTP/1.1", 
        &header_fields, 
        200, 
    };
    char* content_type = "text/html; charset=UTF-8";
    send_response(client_fd, &res_header, body, read_bytes, content_type);
    free(body);
    free_list(&header_fields);
}

void posts_route_handler(int* client_fd, HTTPRequest* req) {
    unsigned char* body = NULL;
    ssize_t read_bytes = load_page(&body, "/posts/index.html");
    if (body == NULL) {
        err("home_route_handler", "Unable to read file content!");
        free(body);
        return;
    }

    List header_fields = {0, NULL};
    HTTPResponseHeader res_header = {
        {0},
        "OK", 
        "HTTP/1.1", 
        &header_fields, 
        200, 
    };
    char* content_type = "text/html; charset=UTF-8";
    send_response(client_fd, &res_header, body, read_bytes, content_type);
    free(body);
    free_list(&header_fields);
}

void not_found_route_handler(int* client_fd, HTTPRequest* req) {
    unsigned char* body = NULL;
    ssize_t read_bytes = load_page(&body, "/404.html");
    if (body == NULL) {
        err("home_route_handler", "Unable to read file content!");
        free(body);
        return;
    }

    List header_fields = {0, NULL};
    HTTPResponseHeader res_header = {
        {0},
        "Not Found",
        "HTTP/1.1",
        &header_fields,
        404
    };
    char* content_type = "text/html; charset=UTF-8";
    send_response(client_fd, &res_header, body, read_bytes, content_type);
    free(body);
    free_list(&header_fields);
}
