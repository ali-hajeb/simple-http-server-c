#ifndef POLLS_H
#define POLLS_H

#include <stdio.h>
#include <poll.h>
#include <sys/poll.h>

typedef struct {
    size_t size;
    size_t max_size;
    struct pollfd* items;
} PollFd;

int init_pfds(PollFd* pfds, size_t initial_size);
int pfds_add(PollFd* pfds, int new_fd);
int pfds_del(PollFd* pfds, int index);
int handle_new_connection(PollFd* pfds, int listener_fd);
ssize_t handle_client_data(int client_fd, char* request_buffer, size_t buffer_size);
void free_pfds(PollFd* pfds);

#endif
