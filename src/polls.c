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
