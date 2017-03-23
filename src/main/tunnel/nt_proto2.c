/**
 * @author caofuxiang
 *         2015-12-30 09:57:57.
 */

#include <string.h>
#include "nt_proto2.h"

#define NT_API2_TAG 89

void nt_api2_header_init(nt_api2_header_t * header,
                         uint32_t id,
                         uint32_t src_ip, uint16_t src_port,
                         uint32_t dst_ip, uint16_t dst_port,
                         nt_encrypt_type_t encrypt_type,
                         uint8_t ttl,
                         uint16_t payload_size) {
    header->tag = NT_API2_TAG;
    header->id_n = htonl(id);
    header->src_ip = src_ip;
    header->src_port = src_port;
    header->dst_ip = dst_ip;
    header->dst_port = dst_port;
    header->encrypt_type = encrypt_type;
    header->ttl = ttl;
    header->payload_size_n = htons(payload_size);
    memset(header->reserved, 0, sizeof(header->reserved));
}

bool nt_api2_header_is_valid(void * header) {
    nt_api2_header_t * h = header;
    return h->tag == NT_API2_TAG
           && h->encrypt_type >= NT_ENCRYPT_TYPE_NONE && h->encrypt_type < NT_ENCRYPT_TYPE_NUM;
}

bool sbuilder_api2_header(sbuilder_t * builder, nt_api2_header_t * header) {
    char src_ip[100];
    char dst_ip[100];
    struct in_addr addr;
    addr.s_addr = header->src_ip;
    strncpy(src_ip, inet_ntoa(addr), sizeof src_ip);
    addr.s_addr = header->dst_ip;
    strncpy(dst_ip, inet_ntoa(addr), sizeof dst_ip);
    return sbuilder_format(
            builder, "{tag=%u, id=%u, src_ip=%s, src_port=%u, dst_ip=%s, dst_port=%u, encrypt_type=%d, ttl=%u, payload_size=%u}",
            header->tag, nt_api2_header_id(header), src_ip, header->src_port, dst_ip, header->dst_port,
            header->encrypt_type, header->ttl,
            nt_api2_header_payload_size(header));
}
