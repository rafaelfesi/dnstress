#ifndef __QUERY_H__
#define __QUERY_H__

#include <stdio.h>

#include <ldns/ldns.h>

#include "dnsconfig.h"
#include "servant.h"

struct servant_t;
struct dnsconfig_t;

struct query_t {
    char *dname;
    int blocked;
    char *type;
};

typedef enum {
    BUFFER_NULL         = -1,
    SERVER_NULL         = -2,
    CONFIG_NULL         = -3,
    SERVANT_NULL        = -4,
    SERVANT_NON_ACTIVE  = -5,
    ANSWER_NULL         = -6,
    QUERY_NULL          = -7,
    DNAME_NULL          = -8,
    PKT2BUFFER_ERROR    = -9,
    QUERY_PKT_NULL      = -10,
    SERVANT_CLEANED     = -11,
    INVALID_ANSWER_SIZE = -12

} phandler_error_t;

typedef ssize_t (*sender_func)(ldns_buffer *qbin, int sockfd, 
    const struct sockaddr_storage *to, socklen_t tolen);

typedef uint8_t * (*receiver_func)();

int query_create(const struct dnsconfig_t *config, ldns_buffer *buffer);
int reply_process(const struct servant_t *servant, const uint8_t *answer,
    const size_t answer_size);
ssize_t perform_query(const struct servant_t *servant, sender_func send_query);
ssize_t recv_reply(const struct servant_t *servant, receiver_func recvr,
    const servant_type_t type);

#endif /* __QUERY_H__ */