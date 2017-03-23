/**
 * @author caofuxiang
 *         2015-08-06 09:52:52.
 */

#include "nt_monitor.h"
#include "nt_client.h"

#define UDP_BUFFER_SIZE (64*1024)

#define PACKET_TYPE_REQUEST 0
#define PACKET_TYPE_RESPONSE 1

#pragma pack(push, 1)
typedef struct {
    uint8_t type;
    uint32_t id;
    uint32_t size;
    uint64_t timestamp;
    uint64_t checksum;
    char payload[0];
} packet_t;
#pragma pack(pop)

static inline uint64_t packet_checksum(packet_t * packet) {
    uint64_t checksum = packet->type;
    checksum = checksum*31 + packet->id;
    checksum = checksum*31 + packet->size;
    checksum = checksum*31 + packet->timestamp;
    return checksum;
}

/*
static void packet_init(packet_t * packet, uint8_t type, uint32_t id, uint32_t size, uint64_t timestamp) {
    packet->type = type;
    packet->id = id;
    packet->size = size;
    packet->timestamp = timestamp;
    packet->checksum = packet_checksum(packet);
}
*/

static void handle_read(dispatcher_t * dispatcher, udp_t * udp, void * extra) {
    nt_monitor_t * monitor = extra;
    char buffer[UDP_BUFFER_SIZE];
    address_t src_addr;
    address_t tunnel_addr;
    uint32_t id;
    size_t received;
    error_t err = udp_recvfrom_tunnel(udp, buffer, sizeof buffer, &src_addr, &id, &tunnel_addr, &received);
    if (err != 0) return;
    packet_t * packet = (packet_t *) buffer;
    if (packet->type == PACKET_TYPE_REQUEST) {
        packet->type = PACKET_TYPE_RESPONSE;
        size_t sent;
        udp_sendto_tunnel(udp, buffer, received, &src_addr, monitor->sent++, &tunnel_addr, &sent);
    } else if (packet->type == PACKET_TYPE_RESPONSE) {
        if (packet->size != received || packet->checksum != packet_checksum(packet)) {
            ++ monitor->invalid;
            return;
        }
        ++ monitor->received;
    }
}

error_t nt_listener_init(nt_monitor_t * monitor, address_t * local_address, size_t packet_size, int qps, perf_t * perf) {
    error_t err = udp_init(&monitor->udp, local_address, true);
    if (err != 0) return err;

    err = dispatcher_init1(&monitor->dispatcher, 100);
    if (err != 0) return err;

    udp_handler_init(&monitor->udp_handler, &monitor->udp, monitor, handle_read, NULL);

    err = dispatcher_add_udp(&monitor->dispatcher, &monitor->udp_handler);
    if (err != 0) return err;

    monitor->packet_size = packet_size;
    monitor->qps = qps;
    monitor->perf = perf;

    monitor->sent = 0;
    monitor->received = 0;
    monitor->invalid = 0;
    return 0;
}
