#ifndef __ARG_PARSER_H__
#define __ARG_PARSER_H__

#include <stdio.h>
#include <stdlib.h>

#include "dnsconfig.h"

#define HELP "--help"
#define CONF "--conf" /* file with dnstress configuration */

/* TODO: think about modes a little bit more */
#define MODE       "--mode"
#define U_VALID    "udp-valid"     /* udp valid DNS request */
#define T_VALID    "tcp-valid"     /* tcp valid DNS request */
#define U_NONVALID "udp-nonvalid"  /* udp non-valid DNS request */
#define T_NONVALID "tcp-nonvalid"  /* tcp non-valid DNS request */
#define _SHUFFLE   "shuffle"       /* any DNS requests */

#define MIN_WCOUNT 1

#define RPS   "--rps"
#define LDLVL "--ld-lvl"

typedef struct m_entity_t m_entity_t;

struct m_entity_t {
    char * key;
    size_t value;
};

void parse_args(const int argc, char **argv,
    struct dnsconfig_t *config);

#endif