#include "../include/polls.h"
#include "../include/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

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

int pfds_del(PollFd* pfds, int index) {
    if (pfds == NULL || index < 0 || index >= (int)pfds->size) {
        return -1;
    }
    if (pfds->items[index].fd >= 0) {
        close(pfds->items[index].fd);
        printf("Closed fd %d at index %d\n", pfds->items[index].fd, index);
    }
    pfds->items[index] = pfds->items[pfds->size - 1];
    pfds->size--;
    if (pfds->size > 0) {
        pfds->items[pfds->size].fd = -1;
        pfds->items[pfds->size].events = 0;
        pfds->items[pfds->size].revents = 0;
    }
    return 1;}

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

ssize_t handle_client_data(int client_fd, char* request_buffer, size_t buffer_size) {
    if (client_fd < 0 || request_buffer == NULL) {
        return -1;
    }

    ssize_t recieved_bytes = recv(client_fd, request_buffer, buffer_size - 1, 0);
    if (recieved_bytes == -1) {
        // err("start_server", "Unable to recieve request data from client!");
        printf("\t[CLIENT#%d] Unable to recieve request data from client!\n", client_fd);
    } else if (recieved_bytes == 0) {
        printf("\t[CLIENT#%d] DISCONNECTED!\n", client_fd);
    }
    request_buffer[recieved_bytes] = '\0';

    return recieved_bytes;
}

void free_pfds(PollFd* pfds) {
    for (size_t i = 0; i < pfds->size; i++) {
        // if (pfds->items[i] != NULL) {
            close(pfds->items[i].fd);
            // free(pfds->items[i]);
        // }
    }
    free(pfds->items);
    pfds->size = 0;
}
