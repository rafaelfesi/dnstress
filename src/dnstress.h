#ifndef __DNSTRESS_H__
#define __DNSTRESS_H__

#include <stdio.h>
#include <stdlib.h>

#include <event2/event.h>

#include "dnsconfig.h"
#include "threadpool.h"
#include "worker.h"

struct dnstress_t {
    dnsconfig_t *config;
    
    struct worker_t *workers;
    size_t    workers_count; /* same as addrs_count */

    thread_pool_t *pool; /* each worker is a threadpool worker */

    struct event_base *evb; /* event base */

    size_t max_senders;
};

struct dnstress_t * dnstress_create(dnsconfig_t *config);
int dnstress_run(struct dnstress_t *dnstress);
int dnstress_free(struct dnstress_t *dnstress);

#endif /* __DNSTRESS_H__ */