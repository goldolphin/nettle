/**
 * @author caofuxiang
 *         2015-10-28 16:04:04.
 */

#ifndef NETTLE_NT_PROXY_H
#define NETTLE_NT_PROXY_H

#include <utils/utils.h>
#include <dispatcher/tun_tap.h>
#include <dispatcher/udp.h>

typedef struct {
    tun_tap_t tun_tap;
    udp_t udp;
    handler_t tun_tap_handler;
    udp_handler_t udp_handler;
    address_t tunnel_addr;
    int tun_tap_buffer_size;
    address_t local_addr;

    idgen_t idgen;
} nt_proxy_t;

error_t nt_proxy_init(nt_proxy_t * proxy, const char * tun_tap_name, address_t * local_addr, address_t * tunnel_addr, int tun_tap_buffer_size);

error_t nt_proxy_destroy(nt_proxy_t * proxy);

error_t dispatcher_add_nt_proxy(dispatcher_t * dispatcher, nt_proxy_t * proxy);

#endif //NETTLE_NT_PROXY_H
