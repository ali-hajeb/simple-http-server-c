#include "../include/polls.h"
#include "../include/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

/*
 * Function: init_pfds
 *
 * -------------------
 *
 *  Initializes a Poll list.
 *
 *  pfds: pointer to the poll file descriptor list struct.
 *  initial_size: list's initial size.
 *
 *  returns: if failed (-1), on success (1).
 */
int init_pfds(PollFd* pfds, size_t initial_size) {
    if (pfds == NULL) {
        return -1;
    }

    pfds->items = malloc(sizeof(struct pollfd) * initial_size);
    if (pfds->items == NULL) {
        err("init_pfds", "Unable to allocate memory for pfds!");
        return -1;
    }

    pfds->max_size = initial_size;
    pfds->size = 0;
    return 1;
}

/*
 * Function pfds_add
 *
 * -----------------
 *
 *  Adds a new poll file descriptor to the list.
 *
 *  pfds: Pointer to the Poll list.
 *  new_fd: File descriptor.
 *
 *  returns: if failed (-1), on success (1).
 */
int pfds_add(PollFd* pfds, int new_fd) {
    if (pfds == NULL || new_fd < 0) {
        return -1;
    }

    if (pfds->size + 1 >= pfds->max_size) {
        size_t new_size = pfds->max_size * 2;
        pfds->items = realloc(pfds->items, new_size * sizeof(struct pollfd));
        if (pfds->items == NULL) {
            err("pfds_add", "Unable to allocate memory for pfds list!");
            return -1;
        }
        printf("pfds resized to max_size=%zu\n", new_size);
        pfds->max_size = new_size;
    }

    pfds->items[pfds->size].fd = new_fd;
    pfds->items[pfds->size].events = POLLIN;
    pfds->items[pfds->size].revents = 0;
    pfds->size++;
    printf("Added fd %d to pfds, size=%zu\n", new_fd, pfds->size);
    return 1;
}

/*
 * Function: pfds_del
 *
 * ------------------
 *
 *  Deletes and closes desired polls item.
 *
 *  pfds: pointer to the polls list.
 *  index: item index.
 *
 *  returns: if failed (-1), on success (1).
 */
int pfds_del(PollFd* pfds, int index) {
    if (pfds == NULL || index < 0 || index >= (int)pfds->size || pfds->size == 0) {
        return -1;
    }

    if (pfds->items[index].fd >= 0) {
        close(pfds->items[index].fd);
        printf("Closed fd %d at index %d\n", pfds->items[index].fd, index);
    }

    if (index < (int)(pfds->size - 1)) {
        pfds->items[index] = pfds->items[pfds->size - 1];
    }

    pfds->size--;

    if (pfds->size > 0) {
        pfds->items[pfds->size].fd = -1;
        pfds->items[pfds->size].events = 0;
        pfds->items[pfds->size].revents = 0;
    }
    return 1;
}

/*
 * Function: handle_new_connection
 *
 * -------------------------------
 *
 *  Handles new coming connection and adds it to the list.
 *
 *  pfds: pointer to the poll list.
 *  listener_fd: listener socket file descriptor.
 *
 *  returns: if failed (-1), on success (1).
 */
int handle_new_connection(PollFd* pfds, int listener_fd) {
    if (pfds == NULL) {
        return -1;
    }

    struct sockaddr_storage client_addr;
    socklen_t client_size = sizeof(client_addr);

    int client_fd = accept(listener_fd, (struct sockaddr*) &client_addr, &client_size);
    if (client_fd == -1) {
        err("handle_new_connection", "Unable to establish a connection with the client!");
        return -1;
    }

    pfds_add(pfds, client_fd);
    return client_fd;
}

/*
 * Function: handle_client_data
 *
 * ----------------------------
 *
 *  Recieves client's request data.
 *
 *  client_fd: client's file descriptor.
 *  request_buffer: pointer to the request data buffer.
 *  buffer_size: pointer to the buffer size.
 *
 *  returns: recieved bytes. if failed (-1).
 */
ssize_t handle_client_data(int client_fd, char* request_buffer, size_t request_buffer_size) {
    if (client_fd < 0 || request_buffer == NULL) {
        return -1;
    }

    size_t recieve_buffer_size = 1024 * sizeof(char);
    char* recieve_buffer = malloc(recieve_buffer_size);
    if (recieve_buffer == NULL) {
        err("handle_client_data", "Unable to allocate memory for buffer!");
        return -1;
    }

    ssize_t recieved_bytes = -1;
    size_t buffer_pos = 0, buffer_size = request_buffer_size;
    while ((recieved_bytes = recv(client_fd, recieve_buffer, recieve_buffer_size - 1, 0)) > 0) {
        if (buffer_pos + recieved_bytes >= buffer_size) {
            size_t new_buffer_size = (buffer_pos + recieved_bytes) * 2 * sizeof(char);
            request_buffer = realloc(request_buffer, new_buffer_size);
            if (request_buffer == NULL) {
                err("handle_client_data", "Unable to re-allocate memory for buffer!");
                free(recieve_buffer);
                return -1;
            }
            buffer_size += new_buffer_size;
        }

        strncat(request_buffer, recieve_buffer, recieved_bytes);
        buffer_pos += recieved_bytes;
    }

    if (recieved_bytes == -1) {
        // err("start_server", "Unable to recieve request data from client!");
        printf("\t[CLIENT#%d] Unable to recieve request data from client!\n", client_fd);
    } else if (recieved_bytes == 0) {
        printf("\t[CLIENT#%d] DISCONNECTED!\n", client_fd);
    }
    request_buffer[buffer_pos + 1] = '\0';

    free(recieve_buffer);
    return recieved_bytes;
}

/*
 * Function: free_pfds
 *
 * -------------------
 *
 *  Frees poll list.
 *
 *  pfds: pointer to the poll list.
 */
void free_pfds(PollFd* pfds) {
if (pfds == NULL) return;
    for (size_t i = 0; i < pfds->size; i++) {
        if (pfds->items[i].fd >= 0) {
            close(pfds->items[i].fd);
        }
    }
    free(pfds->items);
    pfds->items = NULL;
    pfds->size = 0;
    pfds->max_size = 0;
}
