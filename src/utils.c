#include "../include/utils.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
* Function err
* ------------
*  Prints an error message
*
*  func_name: Name of the parent function
*  message: Error message text
*/
void err(const char* func_name, const char* message) {
    fprintf(stderr, "\n[ERROR]: %s() {} -> %s\n", func_name, message);
}

/*
* Function open_file
* ------------------
*  Returns a file pointer
*
*  path: File path
*  mode: fopen modes
*
*  returns: Pointer to the file. If failed, returns NULL
*/
FILE* open_file(const char* path, const char* mode) {
    FILE* file = fopen(path, mode);
    if (file == NULL) {
        fprintf(stderr, "\n[ERROR]: open_file() {} -> Unable to open '%s'!\n", path);
        return NULL;
    }
    return file;
}

/*
* Function: extract_filename_format
* ---------------------------------
*  Extracts filename and extention from file path
*  
*  filepath: Pointer to the file path
*  filename: Pointer to the filename char_pointer
*  fileformat: Pointer to the fileformat char_pointer
*
*  returns: If failed (-1), Only file name (1), Only extention (2), both filename & extention (3)
*/
int extract_filename_format(const char *filepath, char **filename, char **fileformat) {
	int result = -1;
	const char *p = filepath;
	ssize_t last_slash_pos = -1;
	ssize_t last_dot_pos = -1;

	// Finding last slash and last dot
	size_t pos = 0;
	while (*p != '\0') {
		if (*p == '/' || *p == '\\') {
			last_slash_pos = pos;
		}
		if (*p == '.') {
			last_dot_pos = pos;
		}

		pos++;
		p++;
	}

	// Checking if last slash was found
	if (last_slash_pos == -1) {
		printf("\n[ERROR]: extract_filename_format() {} -> Invalid file path! (All file path must contain '/' or '\\'.)\n");
		printf("EXAMPLES:\n\t- C:\\...\\[file]\n\t- ./[file]\n\r");
		return -1;
	}
	// Checking if last dot was found
	if (last_dot_pos < 1) {
		// Assigning filepath end position to the last_dot_pos
		last_dot_pos = pos;
	}

	// Checking if file name has one or more characters. (Some files might be like '.ext')
	size_t filename_size = last_dot_pos - last_slash_pos - 1;
	if (filename_size >= 1) {
		*filename = malloc(filename_size + 1);
		if (*filename == NULL) {
			printf("\n[ERROR]: extract_filename_format() {} -> Unable to allocate memory for filename!\n");
			return -1;
		}

		// Getting file name
		for (size_t i = 0; i < filename_size; i++) {
			(*filename)[i] = filepath[i + last_slash_pos + 1];
		}
		(*filename)[filename_size] = '\0';
		// printf("%s\n", *filename);
		// Setting result = 0
		result = 0;
	}

	// Checking if file has an extension
	if (last_dot_pos > -1 && pos - last_dot_pos > 1) {
		size_t fileformat_size = pos - last_dot_pos - 1;
		*fileformat = malloc(fileformat_size + 1);
		if (*fileformat == NULL) {
			printf("\n[ERROR]: extract_filename_format() {} -> Unable to allocate memory for fileformat!\n");
			return -1;
		}

		// Getting file format
		for (size_t i = 0; i < fileformat_size; i++) {
			(*fileformat)[i] = filepath[i + last_dot_pos + 1];
		}
		(*fileformat)[fileformat_size] = '\0';
		// printf("%s\n", *fileformat);

		// Setting the appropriate result
		// If file name was found:
		if (result == 0) {
			result = 2;
		}
		// If file only has extension
		else {
			result = 1;
		}
	}

	return result;
}

/*
* Function: get_file_size
* -----------------------
*  Returns the size of the file
*
*  file: Pointer to the file
*
*  returns: file size
*/
size_t get_file_size(FILE* file) {
    size_t current_pos = ftell(file);
    fseek(file, 0, SEEK_END);
    size_t end = ftell(file); 
    fseek(file, current_pos, SEEK_SET);
    return end;
}

/*
* Function get_line
* -----------------
*  Reads a line from stream.
*
*  line_ptr: Pointer to the char array that stores the line.
*  size: Initial size of the input buffer.
*  stream: Pointer to the input stream (stdin, file, etc).
*
*  returns: Number of read characters. If failed, (-1).
*/
ssize_t get_line(char **line_ptr, size_t *size, FILE *stream) {
	// Checking if all parameters are valid and provided;
	if (line_ptr == NULL || size == NULL || stream == NULL) {
		printf("[ERROR]: get_line() {} -> Required parameters are not valid!\n");
		return -1;
	}

	// Defining cursor and input character
	size_t pos = 0;
	int ch;

	// Checking if buffer is allocated and if not, allocate a buffer
	if (*line_ptr == NULL) {
		*size = 128; // Initial buffer size
		*line_ptr = malloc(*size);
		if (*line_ptr == NULL) {
			printf("[ERROR]: get_line() {} -> Unable to initialize a buffer for input!\n");
			return -1;
		}
	}

	while ((ch = fgetc(stream)) != EOF) {
		// Checking if more memory is needed for buffer
		if (pos + 1 >= *size) {
			// Allocating more memory for buffer
			size_t new_size = 2 * (*size);
			char *temp = realloc(*line_ptr, new_size);
			if (temp == NULL) {
				printf("[ERROR]: get_line() {} -> Unable to reallocate more memory for input buffer!\n");
				return -1;
			}
			*line_ptr = temp;
			*size = new_size;
		}

		// Terminate input if user presses 'ENTER'
		if (ch == '\n') {
			break;
		}
		// Adding the input character to the buffer
		(*line_ptr)[pos++] = ch;
	}

	// Check if no data was read
	if (pos == 0 || ch == EOF) {
		return -1;
	}

	// Terminate the buffer with a null character
	(*line_ptr)[pos] = '\0';

	// Return the size of string
	return pos;
}
