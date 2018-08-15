#include "phandler.h"
#include "utils.h"

#define RECV_TRIES 5
#define SEND_TRIES 5

static struct query_t *
__random_query(struct dnsconfig_t *config)
{
    return &config->queries[randint(config->queries_count)];
}

static struct query_t *
get_random_query(struct dnsconfig_t *config)
{
    struct query_t *query = NULL;
    switch (config->mode) {
        case TCP_VALID:
        case UDP_VALID:
            while ((query = __random_query(config))->blocked == 1)
                continue;
            break;
        case UDP_NONVALID:
        case TCP_NONVALID:
            while ((query = __random_query(config))->blocked == 0)
                continue;
            break;
        case SHUFFLE:
            query = __random_query(config);
            break;
    }
    return query;
}

static int
query_init_inner_bf(struct dnsconfig_t *config, ldns_buffer *buffer)
{
    if (config == NULL) return CONFIG_NULL;
    if (buffer == NULL) return BUFFER_NULL;

    struct query_t *query = get_random_query(config);

    if (query == NULL)
        return QUERY_NULL;

    ldns_rr_type qtype = ldns_get_rr_type_by_name(query->type);
    ldns_rdf *dname    = ldns_dname_new_frm_str(query->dname);
    
    if (dname == NULL)
        return DNAME_NULL;

    ldns_buffer_clear(buffer);
    // ldns_buffer_set_capacity(buffer, LDNS_MAX_PACKETLEN);
    // ldns_buffer_set_limit(buffer, LDNS_MAX_PACKETLEN);

    ldns_buffer_write_u16 (buffer, ldns_get_random()); /* id                 */
    ldns_buffer_write_u16 (buffer, 0x0120);            /* flags              */
    ldns_buffer_write_u16 (buffer, 1);                 /* queries            */
    ldns_buffer_write_u16 (buffer, 0);                 /* answers            */
	ldns_buffer_write_u16 (buffer, 0);                 /* authority records  */
	ldns_buffer_write_u16 (buffer, 0);                 /* additional records */
    ldns_dname2buffer_wire(buffer, dname);             /* domain name        */
    ldns_buffer_write_u16 (buffer, qtype);             /* query type         */
	ldns_buffer_write_u16 (buffer, LDNS_RR_CLASS_IN);  /* query class        */

    return 0;
}

static int
query_init_inner(struct dnsconfig_t *config, ldns_buffer *buffer)
{
    if (config == NULL) return CONFIG_NULL;
    if (buffer == NULL) return BUFFER_NULL;

    int err_code = 0;
    ldns_pkt *query_pkt = NULL;

    struct query_t *query = get_random_query(config);

    if (query == NULL)
        return QUERY_NULL;

    ldns_rdf *dname    = ldns_dname_new_frm_str(query->dname);
    ldns_rr_type qtype = ldns_get_rr_type_by_name(query->type);
    
    if (dname == NULL)
        return DNAME_NULL;

    query_pkt = ldns_pkt_query_new(dname, qtype, LDNS_RR_CLASS_IN, LDNS_RD);

    if (query_pkt == NULL) {
        err_code = QUERY_PKT_NULL;
        goto ret;
    }

    if (ldns_pkt_id(query_pkt) == 0)
        ldns_pkt_set_random_id(query_pkt);

    ldns_buffer_clear(buffer);

    if (ldns_pkt2buffer_wire(buffer, query_pkt) != LDNS_STATUS_OK)
        err_code = PKT2BUFFER_ERROR;

ret:
    ldns_pkt_free(query_pkt);
    return err_code;
}

/* creates a query inside of servant's buffer */
int
query_create(struct servant_t *servant)
{
    if (servant->buffer == NULL)
        return BUFFER_NULL;
    if (servant->server == NULL)
        return SERVER_NULL;
    if (servant->config == NULL)
        return CONFIG_NULL;
    
    // query_init_inner_bf(servant->config, servant->buffer);
    if (query_init_inner(servant->config, servant->buffer) < 0)
        fatal("%s: error while initializing a query", __func__);

    return 0;
}

int
perform_query(struct servant_t *servant, sender_func send_query)
{
    if (servant == NULL)         return SERVANT_NULL;
    if (servant->server == NULL) return SERVER_NULL;
    if (servant->buffer == NULL) return BUFFER_NULL;
    if (!servant->active)        return SERVANT_NON_ACTIVE;

    if (query_create(servant) < 0)
        fatal("%s: error while creating a query", __func__);

    ssize_t sent = 0;

    /* try to transmit all data until it will be correctly sent */
    // for (size_t i = 0; i < SEND_TRIES; i++) {
    sent = send_query(servant->buffer,
        servant->fd, &servant->server->addr, servant->server->len);
    if (sent < 0) {
        fatal("worker: %d | servant: %d/%s | sent %ld bytes", 
            servant->worker_base->index, servant->index, 
            type2str(servant->type), sent);
    }// else {
    //     break;
    // }
    // }
    return sent;
}

int
recv_reply(struct servant_t *servant, receiver_func recvr, servant_type_t type) {
    if (servant == NULL)
        return SERVANT_NULL;

    if (type == CLEANED)
        return SERVANT_CLEANED;
    
    if (!servant->active)
        return SERVANT_NON_ACTIVE;

    for (size_t i = 0; i < RECV_TRIES; i++) {
        size_t   answer_size = 0;
        uint8_t *answer      = NULL;

        switch (type) {
            case TCP_TYPE:
                answer = recvr(servant->fd, &answer_size);
                break;
            case UDP_TYPE:
                answer = recvr(servant->fd, &answer_size, NULL, NULL);
                break;
            case CLEANED:
                break;
        }

        if (answer == NULL)
            continue;

        if (reply_process(servant, answer, answer_size) < 0)
            log_warn("%s: failed to process a reply", __func__);

        LDNS_FREE(answer);
        break;
    }
    return 0;
}

int
reply_process(struct servant_t *servant, uint8_t *answer, size_t answer_size)
{
    if (servant == NULL)
        return SERVANT_NULL;
    if (servant->buffer == NULL)
        return BUFFER_NULL;
    if (answer == NULL)
        return ANSWER_NULL;
    if (answer_size < 0 || answer_size >= LDNS_MAX_PACKETLEN)
        return INVALID_ANSWER_SIZE;
    
    struct rstats_t *stats = gstats(servant);

    switch (servant->type) {
        case TCP_TYPE:
            if (inc_rsts_fld(stats, &(stats->n_recv_tcp)) < 0)
                return INC_RSTS_FLD_ERROR;
            break;
        case UDP_TYPE:
            if (inc_rsts_fld(stats, &(stats->n_recv_udp)) < 0)
                return INC_RSTS_FLD_ERROR;
            break;
        default:
            fatal("%s: unknown servant type", __func__);
    }

    ldns_buffer_clear(servant->buffer);
    ldns_buffer_write(servant->buffer, answer, answer_size);

    if (stats_update_buf(stats, servant->buffer) < 0)
        return UPDATE_BUF_ERROR;

    return 0;
}