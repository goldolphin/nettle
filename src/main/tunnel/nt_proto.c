/**
 * @author caofuxiang
 *         2015-06-30 20:36:36.
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "nt_proto.h"

#define NT_API_TAG 87

void nt_api_header_init(nt_api_header_t * header, uint32_t id, uint32_t src_ip, uint16_t src_port, uint32_t dst_ip, uint16_t dst_port, uint16_t payload_size) {
    header->tag = NT_API_TAG;
    header->id_n = htonl(id);
    header->src_ip = src_ip;
    header->src_port = src_port;
    header->dst_ip = dst_ip;
    header->dst_port = dst_port;
    header->payload_size_n = htons(payload_size);
}

void nt_api_header_from(nt_api_header_t * header, nt_api2_header_t * from) {
    header->tag = NT_API_TAG;
    header->id_n = from->id_n;
    header->src_ip = from->src_ip;
    header->src_port = from->src_port;
    header->dst_ip = from->dst_ip;
    header->dst_port = from->dst_port;
    header->payload_size_n = from->payload_size_n;
}

bool nt_api_header_is_valid(void * header) {
    return ((nt_api_header_t *) header)->tag == NT_API_TAG;
}

bool sbuilder_api_header(sbuilder_t * builder, nt_api_header_t * header) {
    char src_ip[100];
    char dst_ip[100];
    struct in_addr addr;
    addr.s_addr = header->src_ip;
    strncpy(src_ip, inet_ntoa(addr), sizeof src_ip);
    addr.s_addr = header->dst_ip;
    strncpy(dst_ip, inet_ntoa(addr), sizeof dst_ip);
    return sbuilder_format(builder, "{tag=%u, id=%u, src_ip=%s, src_port=%u, dst_ip=%s, dst_port=%u, payload_size=%u}",
                    header->tag, nt_api_header_id(header), src_ip, header->src_port, dst_ip, header->dst_port, nt_api_header_payload_size(header));
}
