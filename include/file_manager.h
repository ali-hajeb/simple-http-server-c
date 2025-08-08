#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H
#include "hash.h"

#include <stddef.h>
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

typedef struct file_entry {
    File* file;
    struct file_entry* next;
} FileEntry;

typedef struct {
    FileEntry* entery;
} FileTable;

int load_files(char* base_path, HashTable* file_table);
int req_path_to_local(const char* req_path, size_t req_path_size, char** local_path);
File* get_file(const char* path, HashTable* file_table);
ssize_t read_file_content(const char* path, unsigned char** buffer);
void free_file_table(HashTable* file_table, size_t file_table_size);

#endif
