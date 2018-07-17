#ifndef __WORKER_H__
#define __WORKER_H__

#include <stdio.h>
#include <stdlib.h>

#include <event2/event.h>

#include "network.h"
#include "dnstress.h"

struct worker_t {
    request_mode_t mode;

    struct sockaddr_storage *server;

    struct sender_t *senders;
    size_t senders_count;

    struct dnstress_t *dnstress;
};

struct sender_t {
    size_t index;  /* index in worker's `senders` list */
    int fd;
    struct sockaddr_storage *server;
    struct event *ev_recv;
};

void worker_init(struct dnstress_t *dnstress, size_t index);
void worker_run(void *arg);
void worker_clear(struct worker_t * worker);

#endif /* __WORKER_H__ */