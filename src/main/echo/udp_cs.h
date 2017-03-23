/**
 * @author caofuxiang
 *         2015-05-11 16:22:22.
 */

#ifndef NETTLE_UDP_CS_H
#define NETTLE_UDP_CS_H

#include <dispatcher/udp.h>
#include <utils/logger.h>
#include <tunnel/nt_fec.h>
#include <tunnel/nt_distributor.h>
#include <tunnel/nt_proto2.h>

error_t udp_server(address_t * server_address);

error_t udp_client(nt_fec_t * fec, nt_distributor_t * distributor, int count, int qps, address_t * tun_addr,
                   nt_encrypt_type_t encrypt_type);

#endif //NETTLE_UDP_CS_H
