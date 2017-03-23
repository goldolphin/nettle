/**
 * @author caofuxiang
 *         2015-07-30 09:51:51.
 */

#include "nt_client.h"
#include "nt_proto.h"
#include "nt_error.h"
#include "nt_proto2.h"

error_t udp_sendto_tunnel(udp_t *udp, const void *buf, size_t len, const address_t *dst_addr, uint32_t id, const address_t * tunnel_addr, size_t * sent) {
    nt_api_header_t header;
    struct iovec iov[2];
    iov[0].iov_base = &header;
    iov[0].iov_len = sizeof(nt_api_header_t);
    iov[1].iov_base = (void *) buf;
    iov[1].iov_len = len;
    nt_api_header_init(&header, id, 0, 0, address_get_addr4(dst_addr), address_get_port(dst_addr), (uint16_t) len);
    return udp_send_v(udp, iov, array_size(iov), NULL, tunnel_addr, sent);
}

error_t udp_recvfrom_tunnel(udp_t *udp, void *buf, size_t len, address_t * src_addr, uint32_t *id, address_t * tunnel_addr, size_t * received) {
    nt_api_header_t header;
    struct iovec iov[2];
    iov[0].iov_base = &header;
    iov[0].iov_len = sizeof(nt_api_header_t);
    iov[1].iov_base = buf;
    iov[1].iov_len = len;
    size_t received_raw;
    error_t err = udp_recv_v(udp, iov, array_size(iov), tunnel_addr, NULL, &received_raw);
    if (err != 0) return err;
    if (!nt_api_header_is_valid(&header) || received_raw != nt_api_message_size(&header)) {
        return E_API_INVALID_HEADER;
    }
    struct in_addr addr;
    addr.s_addr = header.src_ip;
    address_init4(src_addr, &addr, header.src_port);
    *id = nt_api_header_id(&header);
    *received = nt_api_header_payload_size(&header);
    return 0;
}

error_t udp_sendto_tunnel2(udp_t *udp, const void *buf, size_t len, const address_t *dst_addr, uint32_t id, const address_t * tunnel_addr,
                           nt_encrypt_type_t encrypt_type, uint8_t ttl, size_t * sent) {
    nt_api2_header_t header;
    struct iovec iov[2];
    iov[0].iov_base = &header;
    iov[0].iov_len = sizeof(nt_api2_header_t);
    iov[1].iov_base = (void *) buf;
    iov[1].iov_len = len;
    nt_api2_header_init(&header, id, 0, 0, address_get_addr4(dst_addr), address_get_port(dst_addr),
                        encrypt_type, ttl,(uint16_t) len);
    return udp_send_v(udp, iov, array_size(iov), NULL, tunnel_addr, sent);
}

error_t udp_recvfrom_tunnel2(udp_t *udp, void *buf, size_t len, address_t * src_addr, uint32_t *id, address_t * tunnel_addr,
                             nt_encrypt_type_t * encrypt_type, uint8_t * ttl, size_t * received) {
    nt_api2_header_t header;
    struct iovec iov[2];
    iov[0].iov_base = &header;
    iov[0].iov_len = sizeof(nt_api2_header_t);
    iov[1].iov_base = buf;
    iov[1].iov_len = len;
    size_t received_raw;
    error_t err = udp_recv_v(udp, iov, array_size(iov), tunnel_addr, NULL, &received_raw);
    if (err != 0) return err;
    if (!nt_api2_header_is_valid(&header) || received_raw != nt_api2_message_size(&header)) {
        return E_API_INVALID_HEADER;
    }
    struct in_addr addr;
    addr.s_addr = header.src_ip;
    address_init4(src_addr, &addr, header.src_port);
    *id = nt_api2_header_id(&header);
    *encrypt_type = (nt_encrypt_type_t) header.encrypt_type;
    *ttl = header.ttl;
    *received = nt_api2_header_payload_size(&header);
    return 0;
}
