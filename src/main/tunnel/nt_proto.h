/**
 * @author caofuxiang
 *         2015-06-30 20:36:36.
 */

#ifndef NETTLE_NT_PROTO_H
#define NETTLE_NT_PROTO_H

#include <stdint.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <utils/sbuilder.h>
#include "nt_proto2.h"

#pragma pack(push, 1)
typedef struct {
    uint8_t tag;          // 协议标识.
    uint32_t id_n;          // 消息唯一标识, 方便跟踪.
    uint32_t src_ip;      // 源IP, 使用网络序, 与inet_addr()返回结果兼容.
    uint16_t src_port;    // 源port, 使用网络序, 与htons()返回结果兼容.
    uint32_t dst_ip;      // 目标IP, 同src_ip.
    uint16_t dst_port;    // 目标port, 同src_port.
    uint16_t payload_size_n; // payload长度 in bytes.
} nt_api_header_t;
#pragma pack(pop)

void nt_api_header_init(nt_api_header_t * header, uint32_t id, uint32_t src_ip, uint16_t src_port, uint32_t dst_ip, uint16_t dst_port, uint16_t payload_size);

void nt_api_header_from(nt_api_header_t * header, nt_api2_header_t * from);

static inline uint32_t nt_api_header_id(nt_api_header_t * header) {
    return ntohl(header->id_n);
}

static inline uint16_t nt_api_header_payload_size(nt_api_header_t * header) {
    return ntohs(header->payload_size_n);
}

bool nt_api_header_is_valid(void * header);

static inline int nt_api_message_size(nt_api_header_t * header) {
    return (int) (sizeof(nt_api_header_t) + nt_api_header_payload_size(header));
}

static inline void * nt_api_message_payload(nt_api_header_t * header) {
    return (void *)header + sizeof(nt_api_header_t);
}

bool sbuilder_api_header(sbuilder_t * builder, nt_api_header_t * header);

#endif //NETTLE_NT_PROTO_H
