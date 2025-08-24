#ifndef POLLS_H
#define POLLS_H
#include "buffer.h"

#include <stdio.h>
#include <poll.h>
#include <sys/poll.h>

typedef struct {
    size_t size;
    size_t max_size;
    struct pollfd* items;
} PollFd;

/*
 * Function: init_pfds
 *
 * -------------------
 *
 *  Initializes a Poll list.
 *
 *  pfds: pointer to the poll file discriptor list struct.
 *  initial_size: list's initial size.
 *
 *  returns: if failed (-1), on success (1).
 */
int init_pfds(PollFd* pfds, size_t initial_size);

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
int pfds_add(PollFd* pfds, int new_fd);

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
int pfds_del(PollFd* pfds, int index);


/*
 * Function: free_pfds
 *
 * -------------------
 *
 *  Frees poll list.
 *
 *  pfds: pointer to the poll list.
 */
void free_pfds(PollFd* pfds);
#endif
