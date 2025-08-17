#include "../include/buffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
* Function: init_string_buffer
*
* ----------------------------
*
*  Initiates and prepares the string buffer.
*
*  buffer: Pointer to the StringBuffer struct.
*  initial_size: Initial capacity of the buffer.
*
*  returns: If failed (0).
*/
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

/*
* Function: write_to_string_buffer
* 
* --------------------------------
*
*  Writes data to the StringBuffer.
*
*  buffer: Pointer to the StringBuffer struct.
*  str: Pointer to the string data.
*  str_size: Size of string data.
*
*  returns: Number of written bytes. If failed (-1).
*/
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

/*
* Function: free_string_buffer
*
* ----------------------------
*
*  Frees the buffer.
*
*  buffer: Pointer to the StringBuffer.
*/
void free_string_buffer(StringBuffer* buffer) {
    if (buffer->data != NULL) {
        free(buffer->data);
    }
}
