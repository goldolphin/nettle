/**
 * @author caofuxiang
 *         2015-07-22 11:26:26.
 */

#include "nt_forwarder.h"

void nt_forwarder_init(nt_forwarder_t * forwarder, nt_path_t * path, udp_t * api_udp, udp_t * tun_udp) {
    forwarder->path = path;
    forwarder->api_udp = api_udp;
    forwarder->tun_udp = tun_udp;
}

bool nt_forwarder_send(nt_forwarder_t *forwarder, nt_api_header_t * header) {
    size_t size = (size_t) nt_api_message_size(header);
    size_t sent;
    if (udp_send(forwarder->tun_udp, header, size, &forwarder->path->src, &forwarder->path->dst, &sent) == 0) {
        ensure(sent == size);
        return true;
    } else {
        return false;
    }
}

bool nt_forwarder_receive(nt_forwarder_t *forwarder, nt_api_header_t * header) {
    size_t size = (size_t) nt_api_message_size(header);
    size_t sent;
    address_t dst;
    address_init4(&dst, (struct in_addr *) &header->dst_ip, header->dst_port);
    if (udp_sendto(forwarder->api_udp, header, size, &dst, &sent) == 0) {
        ensure(sent == size);
        return true;
    } else {
        return false;
    }
}
