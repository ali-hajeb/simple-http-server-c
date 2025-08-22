#include "../include/file_manager.h"
#include "../include/utils.h"

#include <stddef.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

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
int load_files(char* base_path, FileTable* file_table) {
    int file_count = 0;
    size_t path_size = 256 * sizeof(char);
    char* path = malloc(path_size);
    if (path == NULL) {
        err("load_files", "Unable to allocate memory for path!");
        return -1;
    }
    struct dirent* dp;
    DIR* dir = opendir(base_path);
    if(dir == NULL) {
        return -1;
    }

    while ((dp = readdir(dir)) != NULL) {
        if ((strcmp(dp->d_name, ".") == 0) || (strcmp(dp->d_name, "..") == 0)) {
            continue;
        }

        // update base path
        size_t new_path_size = strlen(path) + strlen(base_path) + strlen(dp->d_name) + 2;
        if (new_path_size > path_size) {
            size_t upadated_path_size = path_size * 2 + new_path_size;
            path = realloc(path, upadated_path_size);
            if (path == NULL) {
                err("load_files", "Unable to re-allocate memory for path!");
                return -1;
            }
        }
        strcpy(path, base_path);
        strcat(path, "/");
        strcat(path, dp->d_name);

        struct stat statbuf;
        if (stat(path, &statbuf) == 0) {
            if (S_ISDIR(statbuf.st_mode)) {
                file_count += load_files(path, file_table);
            } else {
                File* new_file = malloc(sizeof(File));
                new_file->path = malloc(strlen(path) + 1);
                new_file->fullname = malloc(strlen(dp->d_name) + 1);
                if (new_file == NULL || new_file->path == NULL || new_file->fullname == NULL) {
                    err("load_files", "Unable to allocate memory for the new file!");
                    return -1;
                }

                int result = extract_filename_format(path, &new_file->name, &new_file->extension);
                if (result == -1) {
                    err("load_files", "Unable to extract file name and extension!");
                    free(new_file->name);
                    free(new_file->path);
                    free(new_file->extension);
                    free(new_file->fullname);
                    free(new_file);
                    return -1;
                }

                new_file->access_level = 0;
                strcpy(new_file->fullname, dp->d_name);
                strcpy(new_file->path, path);
                // strcpy(new_file->path, base_path);

                int hash_value = hash(path, strlen(path), FILE_TABLE_SIZE);
                FileEntry* file_entry = add_hash_entry(&file_table->entry[hash_value], new_file);
                if (file_entry == NULL) {
                    err("load_files", "Unable to add file to the file table!");
                    free(new_file->name);
                    free(new_file->path);
                    free(new_file->extension);
                    free(new_file->fullname);
                    free(new_file);
                    return -1;
                }
                printf("file: %s %d\n", path, hash_value);
                file_count++;
            }
        }
    }
    closedir(dir);
    return file_count;
}

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
File* get_file(const char* path, FileTable* file_table) {
    if (file_table == NULL || path == NULL) {
        return NULL;
    }
    int hash_value = hash(path, strlen(path), FILE_TABLE_SIZE);
    for (FileEntry* file_entry = file_table->entry[hash_value]; file_entry != NULL; file_entry = file_entry->next) {
        if (strcmp(((File*) file_entry->data)->path, path) == 0) {
            return (File*) file_entry->data;
        }
    }
    return NULL;
}

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
ssize_t read_file_content(const char* path, unsigned char** buffer) {
    if (path == NULL) {
        return -1;
    }

    FILE* file_ptr = fopen(path, "rb");
    if (file_ptr == NULL) {
        err("read_file_content", "File not fount!");
        return -1;
    }

    size_t file_size = get_file_size(file_ptr);
    *buffer = malloc(file_size * sizeof(unsigned char));
    if (*buffer == NULL) {
        err("read_file_content", "Unable to allocate memory for file buffer");
        printf("\tfile: %s\n", path);
        return -1;
    }

    size_t read_bytes = fread(*buffer, sizeof(unsigned char), file_size, file_ptr);
    fclose(file_ptr);
    return read_bytes;
}

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
int req_path_to_local(const char* req_path, size_t req_path_size, char** local_path) {
    if (req_path == NULL) {
        return -1;
    }

    size_t local_path_size = req_path_size + strlen(DEFAULT_SERVER_PATH) + 1;
    *local_path = malloc(local_path_size * sizeof(char));
    if (*local_path == NULL) {
        err("req_path_to_local", "Unable to allocate memory for local path!");
        return -1;
    }

    int written_bytes = snprintf(*local_path, local_path_size, "%s%s", DEFAULT_SERVER_PATH, req_path);
    return written_bytes;
}

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
void free_file_table(FileTable* file_table) {
    FileEntry* next;
    for (size_t i = 0; i < file_table->size; i++) {
        for (FileEntry* j = file_table->entry[i]; j != NULL; j = next) {
            next = j->next;
            File* file = (File*) j->data;
            free(file->name);
            free(file->extension);
            free(file->path);
            free(file->fullname);
            free(file);
            free(j);
        }
    }
    free(file_table->entry);
    file_table->size = 0;
}

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
ssize_t stream_file_content(int client_fd, const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        err("stream_file_content", "Unable to open file!");
        return -1;
    }
    unsigned char* buffer = malloc(4096 * sizeof(unsigned char));
    if (buffer == NULL) {
        err("stream_file_content", "Unable to allocate memory for file buffer!");
        return -1;
    }
    size_t read_bytes;
    ssize_t total_sent = 0;
    while ((read_bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        ssize_t sent = send(client_fd, buffer, read_bytes, 0);
        if (sent == -1) {
            err("stream_file_content", "Unable to send data!");
            fclose(file);
            return -1;
        }
        total_sent += sent;
    }
    fclose(file);
    return total_sent;
}
