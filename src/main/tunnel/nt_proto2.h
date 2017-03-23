/**
 * @author caofuxiang
 *         2015-12-30 09:57:57.
 */

#ifndef NETTLE_NT_PROTO2_H
#define NETTLE_NT_PROTO2_H

#include <stdint.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <utils/sbuilder.h>

/**
 * Nettle tunnel API v2
 */
#pragma pack(push, 1)
typedef struct {
    uint8_t tag;              // 协议标识, 值定为 89.
    uint32_t id_n;            // 消息唯一标识, 网络序, 方便跟踪.
    uint32_t src_ip;          // 源IP, 使用网络序, 与inet_addr()返回结果兼容.
    uint16_t src_port;        // 源port, 使用网络序, 与htons()返回结果兼容.
    uint32_t dst_ip;          // 目标IP, 同src_ip.
    uint16_t dst_port;        // 目标port, 同src_port.
    uint8_t encrypt_type;     // 加密类型, 取值见 nt_encrypt_type_t, 建议简单的ACK消息可不加密
    uint8_t ttl;              // 消息生命期(用于防止无限内部循环发送, 且提供支持多跳转发的可能), 客户端应设置为一个正整数, 建议值为8
    uint16_t payload_size_n;  // payload长度 in bytes, 使用网络序.
    uint8_t reserved[7];      // 保留字段
} nt_api2_header_t;
#pragma pack(pop)

/**
 * Encryption type.
 */
typedef enum {
    NT_ENCRYPT_TYPE_NONE = 0,    // 不加密
    NT_ENCRYPT_TYPE_KC = 1,      // 使用Key Center加密

    NT_ENCRYPT_TYPE_NUM,         // Should not be used by users.
} nt_encrypt_type_t;

void nt_api2_header_init(nt_api2_header_t * header,
                         uint32_t id,
                         uint32_t src_ip, uint16_t src_port,
                         uint32_t dst_ip, uint16_t dst_port,
                         nt_encrypt_type_t encrypt_type,
                         uint8_t ttl,
                         uint16_t payload_size);

static inline uint32_t nt_api2_header_id(nt_api2_header_t * header) {
    return ntohl(header->id_n);
}

static inline uint16_t nt_api2_header_payload_size(nt_api2_header_t * header) {
    return ntohs(header->payload_size_n);
}

static inline void nt_api2_header_set_payload_size(nt_api2_header_t * header, uint16_t payload_size) {
    header->payload_size_n = htons(payload_size);
}

bool nt_api2_header_is_valid(void * header);

static inline int nt_api2_message_size(nt_api2_header_t * header) {
    return (int) (sizeof(nt_api2_header_t) + nt_api2_header_payload_size(header));
}

static inline void * nt_api2_message_payload(nt_api2_header_t * header) {
    return (void *)header + sizeof(nt_api2_header_t);
}

bool sbuilder_api2_header(sbuilder_t * builder, nt_api2_header_t * header);

#endif //NETTLE_NT_PROTO2_H
