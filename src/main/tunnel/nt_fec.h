/**
 * @author caofuxiang
 *         2015-06-26 15:25:25.
 */

#ifndef NETTLE_NT_FEC_H
#define NETTLE_NT_FEC_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include "nt_error.h"

/**
 * FEC header definition.
 */
#pragma pack(push, 1)
typedef struct {
    uint8_t tag;
    uint8_t N;
    uint8_t K;
    uint32_t block_id_n;    // ID for a block.
    uint8_t block_index;  // Index in the block, start from 0, in ascending order.
    uint16_t payload_size_n; // payload size, in bytes.
} nt_fec_header_t;
#pragma pack(pop)

#define NT_DUMMY_FEC_TAG 39
#define NT_DUP_FEC_TAG 49
#define NT_RS_FEC_TAG 69

void nt_fec_header_init(nt_fec_header_t * header, uint8_t tag, uint8_t N, uint8_t K, uint32_t block_id, uint8_t block_index, uint16_t payload_size);

static inline uint32_t nt_fec_header_block_id(nt_fec_header_t * header) {
    return ntohl(header->block_id_n);
}

static inline void nt_fec_header_set_block_id(nt_fec_header_t * header, uint32_t block_id) {
    header->block_id_n = htonl(block_id);
}

static inline uint16_t nt_fec_header_payload_size(nt_fec_header_t * header) {
    return ntohs(header->payload_size_n);
}

static inline int nt_fec_message_size(nt_fec_header_t * header) {
    return (int) (sizeof(nt_fec_header_t) +  nt_fec_header_payload_size(header));
}

static inline void * nt_fec_message_payload(nt_fec_header_t * header) {
    return (void *)header + sizeof(nt_fec_header_t);
}

/**
 * FEC definition.
 */
struct nt_fec_s;
typedef struct nt_fec_s nt_fec_t;
/**
 * Callback when an encoded packet is generated.
 */
typedef error_t (* nt_fec_on_encode_t)(void * extra, nt_fec_header_t * header, uint8_t * payload, int payload_size);

/**
 * Callback when a packet is decoded.
 */
typedef error_t (* nt_fec_on_decode_t)(void * extra, uint8_t * decoded, int size);

typedef error_t (* nt_fec_encode_t)(nt_fec_t * fec, uint8_t *input, int size, void * extra, nt_fec_on_encode_t callback);

typedef error_t (* nt_fec_decode_t)(nt_fec_t * fec, nt_fec_header_t * header, uint8_t *input, int size, void * extra, nt_fec_on_decode_t callback);

struct nt_fec_s {
    nt_fec_encode_t encode;
    nt_fec_decode_t decode;
};

static inline void nt_fec_init(nt_fec_t * fec, nt_fec_encode_t encode, nt_fec_decode_t decode) {
    fec->encode = encode;
    fec->decode = decode;
}

static inline error_t nt_fec_encode(nt_fec_t * fec, uint8_t *input, int size, void * extra, nt_fec_on_encode_t callback) {
    return fec->encode(fec, input, size, extra, callback);
}

static inline error_t nt_fec_decode(nt_fec_t * fec, nt_fec_header_t * header, uint8_t *input, int size, void * extra, nt_fec_on_decode_t callback) {
    return fec->decode(fec, header, input, size, extra, callback);
}

void nt_dummy_fec_init(nt_fec_t * fec);

#endif //NETTLE_NT_FEC_H
