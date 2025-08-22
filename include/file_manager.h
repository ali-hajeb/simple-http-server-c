#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H
#include "hash.h"

#include <stdio.h>

#define FILE_TABLE_SIZE 128
#define DEFAULT_SERVER_PATH "./http_docs"

typedef struct {
    char* fullname;
    char* name;
    char* extension;
    char* path;
    int access_level;
} File;

typedef HashEntry FileEntry;

typedef HashTable FileTable;


/*
* Function: load_files
*
* --------------------
*
*  Recursivly traverses through directories and adds files to the file table.
*
*  base_path: Starting directory.
*  file_table: Pointer to the files hash table.
*
*  returns: Number of loaded files. If failed, returns (-1).
*/
int load_files(char* base_path, FileTable* file_table);

/*
* Function: get_file
*
* ------------------
*
*  Returns a pointer to the File structure in the files hash table.
*
*  path: File path.
*  file_table: Pointer to the files hash table.
*
*  returns: Pointer to the file.
*/
File* get_file(const char* path, FileTable* file_table);

/*
* Function: read_file_content
*
* --------------------------
*
*  Reads the file content into the buffer.
*
*  path: File path.
*  buffer: Pointer to the buffer.
*
*  returns: Number of read bytes. If failed, returns (-1).
*/
ssize_t read_file_content(const char* path, unsigned char** buffer);

/*
* Function: req_path_to_local 
*
* -----------------
*
*  Converts requested path to local path.
*
*  req_path: Requested path.
*  req_path_size: Requested path string size.
*  local_path: Pointer to the local path string.
*
*  returns: Local path string size. If failed, returns (-1).
*/
int req_path_to_local(const char* req_path, size_t req_path_size, char** local_path);

/*
 * Function: free_file_table
 *
 * -------------------------
 *
 *  Frees files hash table.
 *
 *  file_table: Pointer to the files hash table.
 *  file_table_size: Size of the table.
 */
void free_file_table(FileTable* file_table);

/*
* Function: stream_file_content
*
* -----------------------------
*  
*  Sends file to client in chunks.
*
*  client_fd: client file discriptor.
*  path: file path.
*
*  returns: Number of sent bytes.
*/
ssize_t stream_file_content(int client_fd, const char* path);
#endif
