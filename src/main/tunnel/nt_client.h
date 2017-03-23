/**
 * @author caofuxiang
 *         2015-08-06 10:59:59.
 */

#ifndef NETTLE_NT_CLIENT_H
#define NETTLE_NT_CLIENT_H

#include <dispatcher/common.h>
#include <dispatcher/udp.h>
#include "nt_proto2.h"

error_t udp_sendto_tunnel(udp_t *udp, const void *buf, size_t len, const address_t *dst_addr, uint32_t id, const address_t * tunnel_addr, size_t * sent);

error_t udp_recvfrom_tunnel(udp_t *udp, void *buf, size_t len, address_t * src_addr, uint32_t *id, address_t * tunnel_addr, size_t * received);

error_t udp_sendto_tunnel2(udp_t *udp, const void *buf, size_t len, const address_t *dst_addr, uint32_t id, const address_t * tunnel_addr,
                           nt_encrypt_type_t encrypt_type, uint8_t ttl, size_t * sent);

error_t udp_recvfrom_tunnel2(udp_t *udp, void *buf, size_t len, address_t * src_addr, uint32_t *id, address_t * tunnel_addr,
                             nt_encrypt_type_t * encrypt_type, uint8_t * ttl, size_t * received);

#endif //NETTLE_NT_CLIENT_H
