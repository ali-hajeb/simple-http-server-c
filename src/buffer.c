#include "../include/buffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int init_string_buffer(StringBuffer* buffer, size_t initial_size) {
    if (buffer == NULL || initial_size == 0) {
        fprintf(stderr, "\n[ERROR]: init_buffer_from_file() {} -> Required parameters are NULL!\n");
        return 0;
    }

    buffer->data = malloc(sizeof(char) * initial_size);
    if (buffer->data == NULL) {
        fprintf(stderr, "\n[ERROR]: init_buffer_from_file() {} -> Unable to allocate memory for buffer!\n");
        return 0;
    }

    memset(buffer->data, 0, initial_size);

    buffer->size = 0;
    buffer->max_size = initial_size;
    return 1;
}
ssize_t write_to_string_buffer(StringBuffer* buffer, const char* str, size_t str_size) {
    if (buffer == NULL || str == NULL) {
        fprintf(stderr, "\n[ERROR]: write_to_buffer() {} -> Required parameters are NULL!\n");
        return -1;
    }

    // size_t str_size = strlen(str);

    if (buffer->size + str_size >= buffer->max_size) {
        size_t new_size = buffer->size + buffer->max_size + str_size + 1;
        buffer->data = (char*) realloc(buffer->data, new_size * sizeof(char));
        if (buffer->data == NULL) {
            fprintf(stderr, "\n[ERROR]: write_to_buffer() {} -> Unable to reallocate memory for buffer!\n");
            return -1;
        }
    }

    // printf("{} %zu %zu %zu\n", buffer->size, buffer->max_size, str_size);
    strncat(buffer->data, str, str_size);

    buffer->size += str_size;
    return str_size;
}

void free_string_buffer(StringBuffer* buffer) {
    if (buffer->data != NULL) {
        free(buffer->data);
    }
}

// ----------------

int init_file_buffer(FileBuffer* buffer, FILE* file, size_t max_size) {
    if (buffer == NULL || file == NULL || max_size == 0) {
        fprintf(stderr, "\n[ERROR]: init_buffer_from_file() {} -> Required parameters are NULL!\n");
        return 0;
    }

    buffer->data = malloc(sizeof(unsigned char) * max_size);
    if (buffer->data == NULL) {
        fprintf(stderr, "\n[ERROR]: init_buffer_from_file() {} -> Unable to allocate memory for buffer!\n");
        return 0;
    }
    size_t read_bytes = fread(buffer->data, sizeof(unsigned char), max_size, file);
    buffer->size = read_bytes;
    buffer->max_size = max_size;
    buffer->pos = 0;
    return 1;
}

size_t read_chunk(FileBuffer* buffer, FILE* file) {
    size_t read_bytes = fread(buffer->data, sizeof(unsigned char), buffer->max_size, file);
    buffer->size = read_bytes;
    buffer->pos = 0;
    return read_bytes;
}

ssize_t end_of_file_buffer(FileBuffer* buffer) {
    return buffer->size - buffer->pos;
}

void free_file_buffer(FileBuffer* buffer) {
    if (buffer->data != NULL) {
        free(buffer->data);
    }
}

void print_buffer(const unsigned char* buffer, size_t size, int cols) {
    for (size_t i = 0; i < (size / cols + 1); i++) {
        for (int j = 0; j < cols && ((i * cols) + j) < size; j++) {
            printf("%02X  ", buffer[(i * cols) + j]);
            // printf("%c  ", buffer[(i * cols) + j]);
        }
        printf("\n");
    }
}
