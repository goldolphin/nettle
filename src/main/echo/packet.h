/**
 * @author caofuxiang
 *         2015-06-16 19:11:11.
 */

#ifndef NETTLE_PACKET_H
#define NETTLE_PACKET_H

#include <stddef.h>
#include <stdlib.h>
#include <utils/utils.h>
#include <dispatcher/common.h>

#define BUF_LEN 1100 // in bytes(typical size of messages)

static inline size_t count1(uint64_t i) {
    i = (i & 0x5555555555555555UL) + ((i & 0xAAAAAAAAAAAAAAAAUL) >> 1);
    i = (i & 0x3333333333333333UL) + ((i & 0xCCCCCCCCCCCCCCCCUL) >> 2);
    i = (i & 0x0F0F0F0F0F0F0F0FUL) + ((i & 0xF0F0F0F0F0F0F0F0UL) >> 4);
    i = (i & 0x00FF00FF00FF00FFUL) + ((i & 0xFF00FF00FF00FF00UL) >> 8);
    i = (i & 0x0000FFFF0000FFFFUL) + ((i & 0xFFFF0000FFFF0000UL) >> 16);
    i = (i & 0x00000000FFFFFFFFUL) + ((i & 0xFFFFFFFF00000000UL) >> 32);
    return i;
}

#define LOWEST1_FOO(mask, bits) \
if ((i & mask) == 0) { \
n += bits; \
i >>= bits; \
}

static inline int lowest1(uint64_t i) {
    if (i == 0) return -1;
    int n = 0;
    LOWEST1_FOO(0xFFFFFFFFUL, 32);
    LOWEST1_FOO(0xFFFFUL, 16);
    LOWEST1_FOO(0xFFUL, 8);
    LOWEST1_FOO(0xFUL, 4);
    LOWEST1_FOO(0x3UL, 2);
    LOWEST1_FOO(0x1UL, 1);
    return n;
}

/**
 * Packet
 */
#pragma pack(push, 1)
typedef union {
    struct {
        uint64_t seq; // [deprecated]for underlying strategy logic.
        int id;
        long timestamp;
        uint64_t checksum;
        int payload[0];
    };
    char buf[BUF_LEN];
} packet_t;
#pragma pack(pop)

static inline void packet_set_checksum(packet_t *packet) {
    packet->checksum = count1((uint64_t) packet->timestamp) * count1((uint64_t) packet->id) + packet->id;
}

static inline bool packet_check_checksum(packet_t *packet) {
    return packet->checksum == count1((uint64_t) packet->timestamp) * count1((uint64_t) packet->id) + packet->id;
}

static inline void packet_init(packet_t * packet, int id, long timestamp) {
    packet->id = id;
    packet->timestamp = timestamp;
    packet_set_checksum(packet);
    size_t size = sizeof(packet_t);
    size_t payload_size = (size - ((char *)packet->payload-(char *)packet))/sizeof(int);
    for (size_t i = 0; i < payload_size; ++i) {
        packet->payload[i] = rand();
    }
}

/**
 * Buffer for rs_strategy.
 */
typedef struct {
    uint64_t begin_seq;
    uint64_t mask;
    uint32_t *data;
} rs_buffer_t;

static inline void rs_buffer_reset(rs_buffer_t * rs_buffer, size_t T) {
    rs_buffer->begin_seq = 0;
    rs_buffer->mask = 0;
    memset(rs_buffer->data, 0, sizeof(packet_t)*T*sizeof(uint32_t));
}

static inline void rs_buffer_init(rs_buffer_t * rs_buffer, size_t T) {
    rs_buffer->data = new_array(uint32_t, sizeof(packet_t)*T);
    rs_buffer_reset(rs_buffer, T);
}

static inline void rs_buffer_destroy(rs_buffer_t * rs_buffer) {
    free(rs_buffer->data);
}

static inline void build_packet(packet_t * buffer, uint32_t * data, size_t width, size_t j) {
    uint8_t * b = (uint8_t *) buffer;
    for (int i = 0; i < sizeof(packet_t); ++i) {
        b[i] = (uint8_t) data[i*width+j];
    }
}

#endif //NETTLE_PACKET_H
