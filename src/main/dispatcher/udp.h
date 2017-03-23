/**
 * @author caofuxiang
 *         2015-05-06 09:46:46.
 */

#ifndef NETTLE_UDP_H
#define NETTLE_UDP_H

#include "dispatcher.h"

/**
 * A UDP representation.
 */
typedef struct {
    int fd;
} udp_t;

error_t udp_init(udp_t *udp, address_t * local_address /* NULL to avoid binding. */, bool nonblock);

error_t udp_destroy(udp_t * udp);

static inline int udp_fd(udp_t *udp) {
    return udp->fd;
}

error_t udp_sendto(udp_t *udp, const void *buf, size_t len, const address_t * address, size_t * sent);

error_t udp_recvfrom(udp_t *udp, void *buf, size_t len, address_t * address, size_t * received);

error_t udp_send_v(udp_t *udp, const struct iovec * iov, int iovlen, const address_t * src, const address_t * dst, size_t * sent);

error_t udp_recv_v(udp_t *udp, struct iovec * iov, int iovlen, address_t * src, address_t * dst, size_t * received);

error_t udp_send(udp_t *udp, const void *buf, size_t len, const address_t * src, const address_t * dst, size_t * sent);

error_t udp_recv(udp_t *udp, void *buf, size_t len, address_t * src, address_t * dst, size_t * received);

/**
 * UDP callbacks.
 */
typedef void (* udp_handle_read_t)(dispatcher_t * dispatcher, udp_t * udp, void * extra);
typedef void (* udp_handle_write_t)(dispatcher_t * dispatcher, udp_t * udp, void * extra);

/**
 * Handler for udp.
 */
typedef struct {
    handler_t handler;
    udp_t * udp;
    udp_handle_read_t handle_read;
    udp_handle_write_t handle_write;
    void * extra;
} udp_handler_t;

void udp_handler_init(udp_handler_t *handler, udp_t * udp, void * extra,
                      udp_handle_read_t handle_read /* nullable */,
                      udp_handle_write_t handle_write /* nullable */);

error_t dispatcher_add_udp(dispatcher_t * dispatcher, udp_handler_t *handler);

#endif //NETTLE_UDP_H
