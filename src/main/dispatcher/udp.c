/**
 * @author caofuxiang
 *         2015-05-06 09:46:46.
 */

#include <utils/utils.h>
#include <unistd.h>
#include "udp.h"

error_t udp_init(udp_t *udp, address_t * local_address /* NULL to avoid binding. */, bool nonblock) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return errno;
    }

    if (nonblock && enable_nonblock(fd) != 0) {
        return errno;
    }

    // Enable local iface querying.
    int flag = 1;
    int res1 = setsockopt(fd, IPPROTO_IP, IP_PKTINFO, &flag, sizeof flag);
    int res2 = setsockopt(fd, IPPROTO_IP, IPV6_PKTINFO, &flag, sizeof flag);
    // We try to set both options, so avoid short-circuiting!
    if (res1 != 0 && res2 != 0) {
        return errno;
    }

    if (local_address != NULL) {
        if (bind(fd, &local_address->addr, local_address->addrlen) != 0) {
            return errno;
        }
    }

    udp->fd = fd;
    return 0;
}

error_t udp_destroy(udp_t * udp) {
    if (close(udp->fd) == 0) return 0;
    return errno;
}

static void callback (dispatcher_t * dispatcher, int fd, uint32_t event, void * data) {
    udp_handler_t *handler = (udp_handler_t *) data;
    if ((event & EPOLLIN) != 0) {
        handler->handle_read(dispatcher, handler->udp, handler->extra);
    }
    if ((event & EPOLLOUT) != 0) {
        handler->handle_write(dispatcher, handler->udp, handler->extra);
    }
}

error_t udp_sendto(udp_t *udp, const void *buf, size_t len, const address_t * address, size_t * sent) {
    ssize_t s = sendto(udp->fd, buf, len, 0, &address->addr, address->addrlen);
    if (s < 0) {
        return errno;
    }
    *sent = (size_t) s;
    return 0;
}

error_t udp_recvfrom(udp_t *udp, void *buf, size_t len, address_t * address, size_t * received) {
    address->addrlen = sizeof address->addr;
    ssize_t s = recvfrom(udp->fd, buf, len, 0, &address->addr, &address->addrlen);
    if (s < 0) {
        return errno;
    }
    *received = (size_t) s;
    return 0;
}

error_t udp_send_v(udp_t *udp, const struct iovec * iov, int iovlen, const address_t * src, const address_t * dst, size_t * sent) {
    ssize_t size = send_v(udp->fd, iov, iovlen, src, dst);
    if (size < 0) {
        return errno;
    }
    *sent = (size_t) size;
    return 0;
}

error_t udp_recv_v(udp_t *udp, struct iovec * iov, int iovlen, address_t * src, address_t * dst, size_t * received) {
    ssize_t size = recv_v(udp->fd, iov, iovlen, src, dst);
    if (size < 0) {
        return errno;
    }
    *received = (size_t) size;
    return 0;
}

error_t udp_send(udp_t *udp, const void *buf, size_t len, const address_t * src, const address_t * dst, size_t * sent) {
    struct iovec iov[1];
    iov[0].iov_base = (void *) buf;
    iov[0].iov_len = len;
    return udp_send_v(udp, iov, array_size(iov), src, dst, sent);
}

error_t udp_recv(udp_t *udp, void *buf, size_t len, address_t * src, address_t * dst, size_t * received) {
    struct iovec iov[1];
    iov[0].iov_base = buf;
    iov[0].iov_len = len;
    return udp_recv_v(udp, iov, array_size(iov), src, dst, received);
}

void udp_handler_init(udp_handler_t *handler, udp_t * udp, void * extra,
                      udp_handle_read_t handle_read /* nullable */,
                      udp_handle_write_t handle_write /* nullable */) {
    uint32_t events = 0;
    if (handle_read) {
        events |= EPOLLIN;
    }
    if (handle_write) {
        events |= EPOLLOUT;
    }
    handler_init(&handler->handler, udp->fd, events, callback, handler);

    handler->udp = udp;
    handler->extra = extra;
    handler->handle_read = handle_read;
    handler->handle_write = handle_write;
}

error_t dispatcher_add_udp(dispatcher_t * dispatcher, udp_handler_t *handler) {
    return dispatcher_add_handler(dispatcher, handler->udp->fd, &handler->handler);
}
