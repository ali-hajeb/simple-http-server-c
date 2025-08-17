#ifndef BUFFER_H
#define BUFFER_H
#include <stdio.h>

typedef struct {
    char* data;
    size_t size;
    size_t max_size;
} StringBuffer;

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
int init_string_buffer(StringBuffer* buffer, size_t initial_size);

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
ssize_t write_to_string_buffer(StringBuffer* buffer, const char* data, size_t data_size);

/*
* Function: free_string_buffer
*
* ----------------------------
*
*  Frees the buffer.
*
*  buffer: Pointer to the StringBuffer.
*/
void free_string_buffer(StringBuffer* buffer);
#endif
