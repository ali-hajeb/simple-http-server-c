#ifndef BUFFER_H
#define BUFFER_H
#include <stdio.h>

typedef struct {
    char* data;
    size_t size;
    size_t max_size;
} StringBuffer;

int init_string_buffer(StringBuffer* buffer, size_t initial_size);
ssize_t write_to_string_buffer(StringBuffer* buffer, const char* data, size_t data_size);
void free_string_buffer(StringBuffer* buffer);

typedef struct {
    unsigned char* data;
    size_t pos;
    size_t size;
    size_t max_size;
} FileBuffer;

int init_file_buffer(FileBuffer* buffer, FILE* file, size_t read_size);
void free_file_buffer(FileBuffer* buffer);
size_t read_chunk(FileBuffer* buffer, FILE* file);
ssize_t end_of_file_buffer(FileBuffer* buffer);
void print_buffer(const unsigned char* buffer, size_t size, int cols);
#endif
