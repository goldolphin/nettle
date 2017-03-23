/**
 * @author caofuxiang
 *         2015-07-22 11:26:26.
 */

#ifndef NETTLE_NT_FORWARDER_H
#define NETTLE_NT_FORWARDER_H

#include <dispatcher/udp.h>
#include "nt_distributor.h"
#include "nt_proto.h"

typedef struct {
    nt_path_t * path;
    udp_t * api_udp;
    udp_t * tun_udp;
} nt_forwarder_t;

void nt_forwarder_init(nt_forwarder_t * forwarder, nt_path_t * path, udp_t * api_udp, udp_t * tun_udp);

bool nt_forwarder_send(nt_forwarder_t *forwarder, nt_api_header_t * header);

bool nt_forwarder_receive(nt_forwarder_t *forwarder, nt_api_header_t * header);

#endif //NETTLE_NT_FORWARDER_H
