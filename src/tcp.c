#include "tcp.h"
#include "servant.h"
#include "log.h"
#include "utils.h"

static ssize_t
__send_query(ldns_buffer *qbin, int sockfd,
    const struct sockaddr_storage *to, socklen_t tolen)
{
    uint8_t *sendbuf = NULL;

    ssize_t datasize  = ldns_buffer_position(qbin);
    ssize_t bytes     = 0;
    ssize_t total     = 0;
    ssize_t bytesleft = datasize + 2;

    sendbuf = LDNS_XMALLOC(uint8_t, datasize + 2);
    
    if (sendbuf == NULL)
        fatal("%s: failed to xmalloc sendbuf", __func__);

    ldns_write_uint16(sendbuf, datasize);
    memcpy(sendbuf + 2, ldns_buffer_begin(qbin), datasize);

    while (total < datasize) {
        bytes = send(sockfd, sendbuf + total, bytesleft, 0);
        if (bytes < 0) {
            break;
        }
        
        total     += bytes;
        bytesleft -= bytes;
    }

    LDNS_FREE(sendbuf);

    if (bytes < 0) {
        if (errno == EPIPE)
            return -3;
        return -1;
    }
    if (total != datasize + 2)
        return -2;

    return total;
}

ssize_t
send_tcp_query(const struct servant_t *servant)
{
    ssize_t sent = 0;

    if (!servant->worker_base->active)
        return -1;

    if ((sent = perform_query(servant, __send_query)) < 0) {
        log_warn("%s: error perfoming query | sent %d", __func__, sent);
        return -1;
    }
    if (sent > 0) {
        struct serv_stats_t *sv_stats = get_serv_stats_servant(servant);
        inc_rsts_fld(sv_stats, &sv_stats->tcp_serv.n_sent);
    }
    return sent;
}

void
recv_tcp_reply(evutil_socket_t fd, short events, void *arg)
{
    struct servant_t *servant = (struct servant_t *) arg;

    if (recv_reply(servant, ldns_tcp_read_wire_timeout, TCP_TYPE) < 0)
        fatal("%s: error while receiving reply", __func__);
}